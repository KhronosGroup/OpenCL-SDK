#pragma once

#include <CL/opencl.hpp>

#include <CL/SDK/Image.hpp>
#include <CL/SDK/Options.hpp>

// STL includes
#include <fstream>

class BlurCppExample {
public:
    BlurCppExample(int argc, char* argv[])
        : gauss_kernel(nullptr), origin({ 0, 0 })
    {
        parse_command_line(argc, argv);
    }

    ~BlurCppExample() { delete[] gauss_kernel; }

    void single_pass_box_blur();

    void dual_pass_box_blur();

    void dual_pass_local_memory_exchange_box_blur();

    void dual_pass_subgroup_exchange_box_blur();

    void dual_pass_kernel_blur();

    void dual_pass_local_memory_exchange_kernel_blur();

    void dual_pass_subgroup_exchange_kernel_blur();

    void load_device();

    void read_input_image();

    void prepare_output_image();

    // Query device and runtime capabilities
    std::tuple<bool, bool, bool> query_capabilities();

    void create_image_buffers();

    void build_program(std::string kernel_op);

    void create_gaussian_kernel();

    // Returns true if an option is passed with "-b option" on the command line
    // or if no option is passed.
    bool option_active(std::string option);

    // Sample-specific option
    struct BlurOptions
    {
        std::string in;
        std::string out;
        float size;
        std::vector<std::string>
            op; // This is a vector because MultiArg method is used
    };

private:
    cl::Device device;
    cl::Context context;
    cl::CommandQueue queue;
    std::ifstream kernel_stream;
    cl::Program program;

    cl::sdk::Image input_image;
    cl::Image2D input_image_buf;
    cl::sdk::Image output_image;
    cl::Image2D output_image_buf;
    cl::Image2D temp_image_buf;
    cl::ImageFormat format;
    std::array<cl::size_type, 2> origin;
    std::array<cl::size_type, 2> image_size;
    cl::size_type width;
    cl::size_type height;
    std::string filename;

    bool verbose;
    cl_uint step;

    cl::sdk::options::Diagnostic diag_opts;
    cl::sdk::options::SingleDevice dev_opts;
    BlurOptions blur_opts;

    cl::Buffer gauss_kernel_buf;
    float* gauss_kernel;
    int gauss_size;

    void parse_command_line(int argc, char* argv[]);
    void show_format(cl::ImageFormat* format);
    cl::ImageFormat set_image_format();
    void finalize_blur();

    // note that the kernel is not normalized and has size of 2*(*size)+1
    // elements
    static void create_gaussian_kernel_(float radius, float** const kernel,
                                        int* const size);
    static float gaussian(float x, float radius);
    static void print_timings(std::chrono::duration<double> host_duration,
                              std::vector<cl::Event>& events);
};
