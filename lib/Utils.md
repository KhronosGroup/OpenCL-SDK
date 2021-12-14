# OpenCL Utility Library

The OpenCL Utility Library provides both C and C++ bindings with near feature parity. The utilities are broken into to libraries, `OpenCLUtils` and `OpenCLUtilsCpp`. To include them in your project, include `<CL/Utils/Utils.h>`/`<CL/Utils/Utils.hpp>` and link to their libraries respectively.

## List of utilities

- [Platform](#platform-utilities)
- [Device](#device-utilities)
- [Context](#context-utilities)
- [Event](#event-utilities)
- [Error](#error-handling-utilities)

### Platform utilities

```c++
bool cl::util::supports_extension(const cl::Platform& platform, const cl::string& extension);
```
Tells whether a platform supports a given extension.
- `platform` is the platform to query.
- `extension` is the extension string being searched for.

```c++
bool cl::util::platform_version_contains(const cl::Platform& platform, const cl::string& version_fragment);
```
Tells whether the platform version string contains a specific fragment.
- `platform` is the platform to query.
- `version_fragment` is the version string fragment to search for.

### Device utilities

```c++
bool cl::util::opencl_c_version_contains(const cl::Device& device, const cl::string& version_fragment);
```
Tells whether the device OpenCL C version string contains a specific fragment.
- `device` is the device to query.
- `version_fragment` is the version string fragment to search for.

```c++
bool cl::util::supports_extension(const cl::Device& device, const cl::string& extension);
```
Tells whether a device supports a given extension.
- `device` is the device to query.
- `extension` is the extension string being searched for.

```c++
bool cl::util::supports_feature(const cl::Device& device, const cl::string& feature_name);
```
Tells whether a device supports any version of a feature.
- `device` is the device to query.
- `feature_name` is the feature name string being searched for.

_(Note: this function is only available when both the Utility library and the using code defines minimally `CL_VERSION_3_0`.)_
### Context utilities

```c
cl_context cl_util_get_context(int plat_id, int dev_id, cl_device_type type, cl_int* error);
```
```c++
cl::Context cl::util::get_context(int plat_id, int dev_id, cl_device_type type, cl_int* error = nullptr);
```

These functions create a context on the platform with id `plat_id` with a single device of type `type` and id `dev_id`.

If `error` is non-null or if `CL_HPP_ENABLE_EXCEPTIONS` is used, ordinary OpenCL error codes may be returned and the following library-specific error codes:

- `CL_UTIL_INDEX_OUT_OF_RANGE` if the requested platform or device id is outside the range of available platforms or devices of the selected `type` on the platform.

### Event utilities

```c++
template <cl_int From, cl_int To, typename Dur = std::chrono::nanoseconds>
auto cl::util::get_duration(cl::Event& ev);
```

This function template can be used to query an event for the duration of time measured in user-provided units between two state transitions. By default the return type is `std::chrono::nanoseconds` as that is the unit of measure of the OpenCL API.

### Error handling utilities

```c++
class Error : public std::exception
{
private:
    int err_;
    const char * errStr_;
public:
    /*! \brief Create a new SDK error exception for a given error code
     *  and corresponding message.
     *
     *  \param err error code value.
     *
     *  \param errStr a descriptive string that must remain in scope until
     *                handling of the exception has concluded.  If set, it
     *                will be returned by what().
     */
    Error(cl_int err, const char * errStr = NULL) : err_(err), errStr_(errStr)
    {}

    ~Error() throw() {}

    /*! \brief Get error string associated with exception
     *
     * \return A memory pointer to the error message string.
     */
    virtual const char * what() const throw ()
    {
        if (errStr_ == NULL) {
            return "empty";
        }
        else {
            return errStr_;
        }
    }

    /*! \brief Get error code associated with exception
     *
     *  \return The error code.
     */
    cl_int err(void) const { return err_; }
};
```
This type is used as the exception type thrown by utilities when an error occurs and the compiling code defines `CL_HPP_ENABLE_EXCEPTIONS`
