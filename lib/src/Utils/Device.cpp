#include <CL/Utils/Device.hpp>

#include <algorithm>

bool cl::util::opencl_c_version_contains(const cl::Device& device,
                                         const cl::string& version_fragment)
{
    return device.getInfo<CL_DEVICE_OPENCL_C_VERSION>().find(version_fragment)
        != cl::string::npos;
}

bool cl::util::supports_extension(const cl::Device& device,
                                  const cl::string& extension)
{
    return device.getInfo<CL_DEVICE_EXTENSIONS>().find(extension)
        != cl::string::npos;
}

#ifdef CL_VERSION_3_0
bool cl::util::supports_feature(const cl::Device& device,
                                const cl::string& feature_name)
{
    auto c_features = device.getInfo<CL_DEVICE_OPENCL_C_FEATURES>();
    auto feature_is_work_group_reduce = [&](const cl_name_version& name_ver) {
        return cl::string{ name_ver.name } == feature_name;
    };
    return std::find_if(c_features.cbegin(), c_features.cend(),
                        feature_is_work_group_reduce)
        != c_features.cend();
}
#endif
