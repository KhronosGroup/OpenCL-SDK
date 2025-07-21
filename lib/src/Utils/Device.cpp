#include <CL/Utils/Device.hpp>

#include <algorithm>
#include <vector>
#include <string>

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

std::string to_lower(const std::string& s) {
    std::string r = "";
    for(int i=0; i<(int)s.length(); i++) {
        const char c = s.at(i);
        r += c>64&&c<91 ? c+32 : c;
    }
    return r;
}
bool contains(const std::string& s, const std::string& match) {
    return s.find(match)!=std::string::npos;
}
bool contains_any(const std::string& s, const std::vector<std::string>& matches) {
    for(int i=0; i<(int)matches.size(); i++) if(contains(s, matches[i])) return true;
    return false;
}

std::vector<cl::Device> cl::util::get_devices() { // returns all available devices
    std::vector<cl::Device> cl_devices;
    std::vector<cl::Platform> cl_platforms; // get all platforms (drivers)
    cl::Platform::get(&cl_platforms);
    for(int i=0; i<(int)cl_platforms.size(); i++) {
        std::vector<cl::Device> cl_devices_available;
        cl_platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &cl_devices_available);
        for(int j=0; j<(int)cl_devices_available.size(); j++) {
            cl_devices.push_back(cl_devices_available[j]);
        }
    }
    return cl_devices;
}

cl::Device cl::util::select_device_with_most_flops(const std::vector<cl::Device>& cl_devices=cl::util::get_devices()) { // returns device with best floating-point performance
    float best_value = 0.0f;
    int best_i = 0;
    for(int i=0; i<(int)cl_devices.size(); i++) { // find device with highest (estimated) floating point performance
        const std::string name = cl_devices[i].getInfo<CL_DEVICE_NAME>(); // device name
        const std::string vendor = cl_devices[i].getInfo<CL_DEVICE_VENDOR>(); // device vendor
        const int compute_units = (int)cl_devices[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>(); // compute units (CUs) can contain multiple cores depending on the microarchitecture
        const int clock_frequency = (int)cl_devices[i].getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>(); // in MHz
        const bool is_gpu = cl_devices[i].getInfo<CL_DEVICE_TYPE>()==CL_DEVICE_TYPE_GPU;
        const int ipc = is_gpu?2:32; // IPC (instructions per cycle) is 2 for GPUs and 32 for most modern CPUs
        const bool nvidia_192_cores_per_cu = contains_any(to_lower(name), {" 6", " 7", "ro k", "la k"}) || (clock_frequency<1000&&contains(to_lower(name), "titan")); // identify Kepler GPUs
        const bool nvidia_64_cores_per_cu = contains_any(to_lower(name), {"p100", "v100", "a100", "a30", " 16", " 20", "titan v", "titan rtx", "ro t", "la t", "ro rtx"}) && !contains(to_lower(name), "rtx a"); // identify P100, Volta, Turing, A100
        const bool amd_128_cores_per_dualcu = contains(to_lower(name), "gfx10"); // identify RDNA/RDNA2 GPUs where dual CUs are reported
        const float nvidia = (float)(contains(to_lower(vendor), "nvidia"))*(nvidia_192_cores_per_cu?192.0f:(nvidia_64_cores_per_cu?64.0f:128.0f)); // Nvidia GPUs have 192 cores/CU (Kepler), 128 cores/CU (Maxwell, Pascal, Ampere) or 64 cores/CU (P100, Volta, Turing, A100)
        const float amd = (float)(contains_any(to_lower(vendor), {"amd", "advanced"}))*(is_gpu?(amd_128_cores_per_dualcu?128.0f:64.0f):0.5f); // AMD GPUs have 64 cores/CU (GCN, CDNA) or 128 cores/dualCU (RDNA, RDNA2), AMD CPUs (with SMT) have 1/2 core/CU
        const float intel = (float)(contains(to_lower(vendor), "intel"))*(is_gpu?8.0f:0.5f); // Intel integrated GPUs usually have 8 cores/CU, Intel CPUs (with HT) have 1/2 core/CU
        const float arm = (float)(contains(to_lower(vendor), "arm"))*(is_gpu?8.0f:1.0f); // ARM GPUs usually have 8 cores/CU, ARM CPUs have 1 core/CU
        const int cores = (int)((float)compute_units*(nvidia+amd+intel+arm)+0.5f); // for CPUs, compute_units is the number of threads (twice the number of cores with hyperthreading)
        const float tflops = 1E-6f*(float)cores*(float)ipc*(float)clock_frequency; // estimated device floating point performance in TeraFLOPs/s
        if(tflops>best_value) {
            best_value = tflops;
            best_i = i;
        }
    }
    return cl_devices[best_i];
}

cl::Device cl::util::select_device_with_most_memory(const std::vector<cl::Device>& cl_devices=cl::util::get_devices()) { // returns device with largest memory capacity
    int best_value = 0;
    int best_i = 0;
    for(int i=0; i<(int)cl_devices.size(); i++) { // find device with most memory
        const int memory = (int)(cl_devices[i].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>()/1048576ull); // global memory in MB
        if(memory>best_value) {
            best_value = memory;
            best_i = i;
        }
    }
    return cl_devices[best_i];
}
