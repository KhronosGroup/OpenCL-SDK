#include <CL/Utils/Platform.hpp>

bool cl::util::supports_extension(const cl::Platform& platform,
                                  const cl::string& extension)
{
    return platform.getInfo<CL_PLATFORM_EXTENSIONS>().find(extension)
        != cl::string::npos;
}

bool cl::util::platform_version_contains(const cl::Platform& platform,
                                         const cl::string& version_fragment)
{
    return platform.getInfo<CL_PLATFORM_VERSION>().find(version_fragment)
        != cl::string::npos;
}