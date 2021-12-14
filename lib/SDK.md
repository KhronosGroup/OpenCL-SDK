# OpenCL SDK Library

The OpenCL SDK Library hosts both C and C++ utilities which are generally useful for writing OpenCL applications but are either dependency-heavy or contentious. Because these utilities aren't the subject of universal interest, these utilities are _not_ exported, meaning SDK installations won't install their headers nor their libraries. Doing so the [OpenCL Utility Library](./Utils.md) can be kept dependency-free.

The utilities are broken into to libraries, `OpenCLSDK` and `OpenCLSDKCpp`. Samples include `<CL/Utils/Utils.h>`/`<CL/Utils/Utils.hpp>` and link to their libraries respectively.

## List of utilities

- [Command-line Interface](#command-line-interface-utilities)
- [Pseudo Random Number Generation utilities](#pseudo-random-number-generation-utilities)
- [Image utilities](#image-utilities)
- [OpenCL-OpenGL interop utilities](#openCL-openGL-interop-utilities)

### Command-line interface utilities

#### C
```c
struct cl_sdk_options_DeviceTriplet
{
    int plat_index;
    int dev_index;
    cl_device_type dev_type;
};

struct cl_sdk_options_Diagnostic
{
    bool verbose;
    bool quiet;
};

struct cl_sdk_options_SingleDevice
{
    struct cl_sdk_options_DeviceTriplet triplet;
};

struct cl_sdk_options_MultiDevice
{
    struct cl_sdk_options_DeviceTriplet * triplets;
    size_t number;
};
```
#### C++
```c++
struct cl::sdk::DeviceTriplet
{
    int plat_index;
    int dev_index;
    cl_device_type dev_type;
};
struct cl::sdk::Diagnostic
{
    bool verbose,
         quiet;
};
struct cl::sdk::SingleDevice
{
    DeviceTriplet triplet;
};
struct cl::sdk::MultiDevice
{
    cl::vector<DeviceTriplet> triplets;
};
struct cl::sdk::Window
{
    int width;
    int height;
    bool fullscreen;
};
```

The SDK Library deduplicates the storage the result of common CLI argument parsing. These types are used throughout the SDK samples.

#### C++
```c++
template <typename Option>
auto cl::sdk::parse();

template <typename Option, typename... Parsers>
Option cl::sdk::comprehend(Parsers... parsers);

template <typename... Options>
std::tuple<Options...> cl::sdk::parse_cli(int argc, char* argv[], std::string banner = "OpenCL SDK sample template")
```

These functions reduce the boilerplate needed for each sample to introduce their own set of command-line arguments and helps deduplicate the set of common options which most samples inherit.

The first two are the customizable parts where each sample (and the the pre-defined options) specify how CLI arguments should be processed. `cl::sdk::parse()` specifies what the option names are and should return an instance of tuple-like type (`std::get<int>(const type&)` be valid) containing smart-pointer-like types (`ptr.get()` be valid) holding TCLAP arguments and switches. `cl::sdk::comprehend()` takes this tuple already expanded as arguments with names and "makes sense" of the strings of args already validated during parsing and returns a singular struct of the options at hand.

> The types returned by `cl::sdk::parse()` must exactly match the `Parsers...` of `cl::sdk::comprehend()`.

`cl::sdk::parse_cli()` is a simple metaprogram that invokes the `parse` and `comprehend` functions of all the `Options` types it is given in the correct order. First it registers all the command-line arguments and switches, then invokes the parse method of the underlying CLI parser and then invokes the comprehend method of the 

Simplest example is the C++ SAXPY sample adding a single option to control the length of the vector operands. The sample uses a single OpenCL device and respects diagnostic level options as well.

#### C++
```c++
// Sample-specific option
struct SaxpyOptions { size_t length; };

// Add option to CLI parsing SDK utility
template <> auto cl::sdk::parse<SaxpyOptions>(){
    return std::make_tuple(
        std::make_shared<TCLAP::ValueArg<size_t>>("l", "length", "Length of input", false, 1'048'576, "positive integral")
    );
}
template <> SaxpyOptions cl::sdk::comprehend<SaxpyOptions>(
    std::shared_ptr<TCLAP::ValueArg<size_t>> length_arg){
    return SaxpyOptions{
        length_arg->getValue()
    };
}

int main(int argc, char* argv[])
{
    try
    {
        // Parse command-line options
        auto opts = cl::sdk::parse_cli<
                        cl::sdk::options::Diagnostic,
                        cl::sdk::options::SingleDevice,
                        SaxpyOptions>(argc, argv);
        const auto& diag_opts  = std::get<0>(opts);
        const auto& dev_opts   = std::get<1>(opts);
        const auto& saxpy_opts = std::get<2>(opts);
    ...
    }
    // catch clauses
}
```

_(Note: C++17 structured-bindings allows cleaner binding of names to the members of the tuple returned by `cl::sdk::parse_cli`.)_

### Pseudo Random Number Generation utilities

#### C
```c
void cl_sdk_fill_with_random_floats(pcg32_random_t * rng, cl_float * arr, const size_t len);
```
Fills an array with random floats in the range [0, 1).
- `rng` must point to a valid PRNG instance.
- `arr` must be allocated storage long enough to store the results.
- `len` is the count of elements that will be written to `arr`.

```c
void cl_sdk_fill_with_random_floats_range(pcg32_random_t * rng,
    cl_float * arr, const size_t len, const cl_float low, const cl_float hi);
```
Fills an array with random floats in the range [`low`, `hi`).
- `rng` must point to a valid PRNG instance.
- `arr` must be allocated storage long enough to store the results.
- `len` is the count of elements that will be written to `arr`.
- `low` is the low-end of the range.
- `hi` is the high-end of the range.

```c
void cl_sdk_fill_with_random_ints_range(pcg32_random_t * rng,
    cl_int * arr, const size_t len, const cl_int low, const cl_int hi);
```
Fills an array with uniformly distributed numbers in the range [`low`, `hi`]. Uses rejection sampling from uniform bit distribution.
- `rng` must point to a valid PRNG instance.
- `arr` must be allocated storage long enough to store the results.
- `len` is the count of elements that will be written to `arr`.
- `low` is the low-end of the range.
- `hi` is the high-end of the range.

#### C++
```c++
template <typename PRNG, typename... Containers>
void fill_with_random(PRNG&& prng, Containers&&... containers);
```
Fills containers with random numbers using a user-provided PRNG.
- `prng` must be a PRNG, a callable type with `T(void)` signature where `T` is implicitly convertible to the `value_type` of `containers`.
- `containers` must be a container providing a [LegacyOutputIterator](https://en.cppreference.com/w/cpp/named_req/OutputIterator).

### Image utilities
#### C
```c
typedef struct cl_sdk_image
{
    int width, height, pixel_size;
    unsigned char* pixels;
}
cl_sdk_image;
```
#### C++
```c++
struct cl::sdk::Image
{
    int width, height, pixel_size;
    cl::vector<unsigned char> pixels;
};
```
Used to store pixel information of images read/written to/from storage.
- `width` and `height` store the number of pixels the image has in x-y directions respectively.
- `pixel_size` stores the number of bytes used to store a single pixel.
- `pixels` stores the actual pixel data.

#### C
```c
cl_sdk_image cl_sdk_read_image(const char* file_name, cl_int* err);
```
#### C++
```c++
Image cl::sdk::read_image(const char* file_name, cl_int* err);
```
Reads a BMP/JPEG/PNG image from disk.
- `file_name` specifies the absolute or relative path (to the current working-directory) to the image.
- `err` is an optional pointer used to capture error conditions.

Returns an `cl_sdk_image`/`cl::sdk::Image` instance. If an error occurs, the returned image is in an invalid state.

#### C
```c
void cl_sdk_write_image(const char * file_name, const cl_sdk_image * im, cl_int * err);
```
#### C++
```c++
void cl::sdk::write_image(const char* file_name, const Image& image, cl_int* err);
```
Writes a BMP/JPEG/PNG image from disk.
- `file_name` specifies the absolute or relative path (to the current working-directory) to the image.
- `image` is the source of pixel information.
- `err` is an optional pointer used to capture error conditions.
### OpenCL-OpenGL interop utilities

#### C++
```c++
cl::vector<cl_context_properties> cl::sdk::get_interop_context_properties(const cl::Device& plat, cl_int* error = nullptr);
```

This function returns a null-terminated list of context properties required to setup an OpenCL-OpenGL interop context with the currently active OpenGL context.

If `error` is non-null or if `CL_HPP_ENABLE_EXCEPTIONS` is used, ordinary OpenCL error codes may be returned and the following library-specific error codes:

- `CL_UTIL_OS_GL_QUERY_ERROR` if platform-specific errors occur when trying to query for the currently active OpenGL context.

#### C++
```c++
cl::Context cl::sdk::get_interop_context(int plat_id, int dev_id, cl_device_type type, cl_int* error = nullptr);
```

This function creates an interop context on the platform with id `plat_id` with a single device of type `type` and id `dev_id` which is able to share resources with the currently active OpenGL context.

If `error` is non-null or if `CL_HPP_ENABLE_EXCEPTIONS` is used, ordinary OpenCL error codes may be returned and the following library-specific error codes:

- `CL_UTIL_INDEX_OUT_OF_RANGE` if the requested platform or device id is outside the range of available platforms or devices of the selected `type` on the platform.
- `CL_UTIL_OS_GL_QUERY_ERROR` if platform-specific errors occur when trying to query for the currently active OpenGL context.

#### C++
```c++
class cl::sdk::InteropWindow : public sf::Window
{
public:
    explicit InteropWindow(
        sf::VideoMode mode,
        const sf::String& title,
        sf::Uint32 style = sf::Style::Default,
        const sf::ContextSettings& settings = sf::ContextSettings{},
        int platform_id = 0,
        int device_id = 0,
        cl_bitfield device_type = CL_DEVICE_TYPE_DEFAULT
    );

    void run();

protected:
    // Core functionality to be overriden
    virtual void initializeGL() = 0;            // Function that initializes all OpenGL assets needed to draw a scene
    virtual void initializeCL() = 0;            // Function that initializes all OpenCL assets needed to draw a scene
    virtual void updateScene() = 0;             // Function that holds scene update guaranteed not to conflict with drawing
    virtual void render() = 0;                  // Function that does the native rendering
    virtual void event(const sf::Event& e) = 0; // Function that handles render area resize

    cl::Context opencl_context;
    bool cl_khr_gl_event_supported;
};
```
This class encapsulates an interactive window with the content being one OpenGL canvas. It provides a set of functions for the user to override in derived classes.