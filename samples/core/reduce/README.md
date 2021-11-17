# Reduce

## Sample Purpose

This sample intends to demonstrate how to query various extensions applicable in the context of a reduction algorithm, touch up kernel sources at runtime to select the best kernel implementation for the task.

## Key APIs and Concepts

The most important aspect of this sample is querying exntensions in ways compatible both with 1.x, 2.y, and 3.z runtimes which were introduced at different API versions. The sample also queries the compiled kernel for optimal work-group sizes and launches batches of kernels accordingly.

### Application flow

The application once a device is selected queries whether it's capable of executing built-in `work_group_reduce_<op>` and/or `sub_group_reduce_<op>` intrinsics. Depending on whether the user selects sum calculation or minimum search on the random input, the operation is baked into the kernel.

The kernel supports two reduction operations which is selected via the command-line. The core part of the algorithm is only declared in source code, the definition of the key functions are appended to the source code at runtime.

The reduction kernel otherwise is conceptually simple, it reduces the input to a scalar result in potentially multiple steps. In each step, the output size is that of the input divided by max work-group size * 2. Input isn't overwritten, so double buffering is used where the size of the temporary is the size of the input after one reduction. (Note that other reduction algorithms can be devised which don't need temporary storage.)

#### Explicit host-device-host buffer movement

The sample uses the `cl::Buffer{ cl::CommandQueue, Iter, Iter, bool }` overload when creating the input, which still inside the CTOR invokes obtains the context of the queue, creates the buffer with it, followed by a `cl::copy( cl::CommandQueue, Iter, Iter, cl::Buffer )` which copies to the device associated with the queue using `cl::CommandQueue::enqueueMapBuffer`. The temporary (back) buffer is allocated using the `cl::Buffer{ cl::Context, cl_mem_flags, cl::size_type }` CTOR overload to denote that the initial contents are indeterminate.

```c++
cl::Buffer front{ queue, std::begin(arr), std::end(arr), false },
           back{ context, CL_MEM_READ_WRITE, new_size(arr.size()) * sizeof(cl_int) };
```

Fetching the scalar result is done using `cl::copy`:

```c++
cl_int dev_res;
        cl::copy(queue, back, &dev_res, &dev_res + 1);
```

#### Event profiling

While events are multi-purpose in OpenCL, they are the handles through which synchronization and profiling can be done, here we only demonstrate their profiling capabilities. We can safely omit synchronizing the steps of reduction, because our queue is in-order (no `cl::QueueProperties::OutOfOrder`), so commands in our queue are executed in the order of their enqueue and don't overtake.

Event profiling is turned on by creating the queue as such:
```c++
cl::CommandQueue queue{ context, device, cl::QueueProperties::Profiling };
```
We do not have to test for this capability, as it's mandatory for a device to provide profiling capabilities.

To record the execution time of every reduction step, we save the associated events like this:
```c++
std::vector<cl::Event> passes;
while ( need_to_continue )
{
    passes.push_back(reduce(...));
}
cl::WaitForEvents(passes);
```
The `cl::KernelFunctor` utility of the C++ API exposes a kernel launch as a type-checked C++ function object. Calling the kernel returns the event associated with it. (Type safety means that the arguments provided to the kernel otherwise passing a C ABI are checked to match the promise given when the wrapper was created.) Our `reduce` kernel functor was created as such:
```
auto reduce = cl::KernelFunctor<cl::Buffer, cl::Buffer, cl::LocalSpaceArg, cl_uint, cl_int>(program, "reduce");
```
Instead calling finish on the queue, we may as well wait for all events in `passes` to signal completion. (For a complete list of event state, refer to the [Execution Model](https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#_execution_model) chapter of the OpenCL API spec).

Because most often we measure durations between event state transitions, we use the utility `cl::util::get_duration<From, To, Duration>(cl::Event&)` to get a `std::chrono::duaration` object between the states `From` and `To`. The utility defaults to returing `std::chrono::nanoseconds`, as that is what the OpenCL API uses to store timepoints, but we can request any unit of measurement, such as in this sample:
```c++
for (auto& pass : passes)
    std::cout << "\t" <<
        cl::util::get_duration<
            CL_PROFILING_COMMAND_START,
            CL_PROFILING_COMMAND_END,
            std::chrono::microseconds
        >(pass).count() <<
        " us." << std::endl;
```
_(Note: the main reason why net kernel execution time doesn't amount to the time measured by the host are due to dispatching kernel binaries to the device which happen on the first execution, as the sample doesn't invoke a so called warm-up kernel. By doing so, one can reduce the difference to minimal runtime overhead.)_

## Kernel logic

The sample implements a special case of reduction, where the binary operation can meaningfully operate on any combination of input data and accumulators. (For the non-special case interface, refer to [`std::reduce`](https://en.cppreference.com/w/cpp/algorithm/reduce).) The kernel holds three implementations which are all enabled/disabled based on host side queries.

### Vanilla work-group reduction

When no relevant extensions are supported, the sample does the textbook tree-like reduction.

```cl
int read_local(local int* shared, size_t count, int zero, size_t i)
{
    return i < count ? shared[i] : zero;
}

kernel void reduce(
    global int* front,
    global int* back,
    local int* shared,
    unsigned int length,
    int zero_elem
)
{
    const size_t lid = get_local_id(0),
                 lsi = get_local_size(0),
                 wid = get_group_id(0),
                 wsi = get_num_groups(0);

    const size_t wg_stride = lsi * 2,
                 valid_count = wid != wsi - 1 ? // If not last group
                    wg_stride :                 // as much as possible
                    length - wid * wg_stride;   // only the remaining

    // Copy real data to local
    event_t read;
    async_work_group_copy(
        shared,
        front + wid * wg_stride,
        valid_count,
        read);
    wait_group_events(1, &read);
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int i = lsi; i != 0; i /= 2)
    {
        if (lid < i)
            shared[lid] =
                op(
                    read_local(shared, valid_count, zero_elem, lid),
                    read_local(shared, valid_count, zero_elem, lid + i)
                );
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    if (lid == 0) back[wid] = shared[0];
}
```

Every workgroup loads their share of input into local memory using `async_workgroup_copy`. The last work-group may load slightly less if input size isn't divisible by the max work-group size. Whenever a kernel changes between read/write mode of local memory, a barrier is needed syncing local memory operations denoted by `CLK_LOCAL_MEM_FENCE`.

A decreasing number of work-items participate in each loop iteration. Initially all threads are active, until only one thread is left. `read_local` takes care of uniformity in the algorithmic scheme, even though not every thread may have valid inputs. Threads without valid input data read `zero_elem` instances, leaving the result unchanged.

### Work-group reduction built-in

A simpler implementation is changing the loop to instead invoke a similar work-group collective function like `async_work_group_copy`, but this time `work_group_reduce_<op>`, that way the kernel ends like:
```cl
    int temp = work_group_reduce_op(
        op(
            read_local(shared, valid_count, zero_elem, lid),
            read_local(shared, valid_count, zero_elem, lid + lsi)
        )
    );
if (lid == 0) back[wid] = temp;
```
(Note that `work_group_reduce_op` will be an inline relay to `work_group_reduce_min` or `work_group_reduce_add` based on user preference via command-line options.)

### Sub-group reduction built-in

A slightly more complex variant of the vanilla implementation is provided when the device support sub-group operations. The loop doing the reduction at the end of the kernel then looks like:
```cl
    const uint sid = get_sub_group_id();
    const uint ssi = get_sub_group_size();
    const uint slid= get_sub_group_local_id();
    for(int i = valid_count ; i != 0 ; i /= ssi*2)
    {
        int temp = zero_elem;
        if (sid*ssi < valid_count)
        {
            temp = sub_group_reduce_op(
                op(
                    read_local(shared, i, zero_elem, sid * 2 * ssi + slid),
                    read_local(shared, i, zero_elem, sid * 2 * ssi + slid + ssi)
                )
            );
        }
        barrier(CLK_LOCAL_MEM_FENCE);
        if (sid*ssi < valid_count)
            shared[sid] = temp;
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    if (lid == 0) back[wid] = shared[0];
```
Every sub-group (typically coinciding with some SIMD-like width of the executing device) having at least one valid input performs a sub-group collective primitive, `sub_group_reduce_<op>`. Communicating the results between sub-groups is done via local memory. Because typically sub-groups correlate to the SIMD natue of the executing hardware, syncing work-items in sub-groups is often for free (lockstep execution) or cheaper than syncing an entire work-group. By doing sub-group reducing the input, every loop divides the input size by sub-group size * 2, as opposed to the vanilla algorithm dividing by 2 only. If the sub-group size is larger than 2, then the two barriers required for this particular implementation will still result in a net decrease in the number of work-group barriers issued, resulting in faster execution on most architectures, especially wide SIMD architectures.
### Used API surface

```c++
cl::util::get_context(cl::util::Triplet)
cl::Context::getInfo<CL_CONTEXT_DEVICES>()
cl::CommandQueue(cl::Context, cl::Device)
cl::Device::getInfo<CL_DEVICE_PLATFORM>()
cl::Platform::getInfo<CL_DEVICE_PLATFORM>()
cl::util::get_program(cl::Context, cl::string)
cl::Kernel.getWorkGroupInfo<...>()
cl::KernelFunctor<...>(cl::Program, const char*)
cl::sdk::fill_with_random(...)
cl::Buffer(cl::CommandQueue, Iter, Iter, bool)
cl::copy(cl::CommandQueue, cl::Buffer, Iter, Iter)
```
