/*
 * Copyright (c) 2023 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "blur.hpp"

// STL includes
#include <tuple>
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        // Parse command line arguments and store the parameters in blur class.
        // You can pass '-b box' or '-b gauss' to select conversion type.
        // You can pass both options with "-b box -b gauss" or don't pass
        // anything. If you don't pass a parameter both conversions will be
        // performed.
        BlurCppExample blur(argc, argv);

        // Create a context and load the device used for blur operations.
        blur.load_device();

        // Read input image. If not specified on the command line, the default
        // is generated. If this function fails, ensure that the default file or
        // the one specified on the command line is available. The default image
        // is placed in a default_image.h.
        blur.read_input_image();

        // Prepare output image. It will have the same dimensions as the input
        // image.
        blur.prepare_output_image();

        // Query device and runtime capabilities
        bool use_local_mem, use_subgroup_exchange,
            use_subgroup_exchange_relative;
        std::tie(use_local_mem, use_subgroup_exchange,
                 use_subgroup_exchange_relative) = blur.query_capabilities();

        // Create image buffers used for operation. In this example input,
        // output and temporary image buffers are used. Temporary buffer is used
        // when 2 blur operations in the row are performed. Result of the first
        // operation is stored in temporary buffer and temporary buffer is used
        // as input for 2nd operation.
        blur.create_image_buffers();

        // Create kernel and build program for selected device and blur.cl file
        // without any options. If this function fails, ensure that the blur.cl
        // file is available in place of execution.
        blur.build_program("");

        // The box blur operation will be performed if you pass "-b box" or
        // don't select any option.
        if (blur.option_active("box"))
        {
            // Basic blur operation using a kernel functor. Using kernel
            // functors is more convenient than creating a kernel class object
            // and setting the arguments one by one.
            blur.single_pass_box_blur();

            // Dual pass demonstrates the use of the same kernel functor twice.
            // The result of the first operation is stored in a temporary buffer
            // and used for the second operation.
            blur.dual_pass_box_blur();

            // Comparison with other examples shows the classic approach to
            // working with local memory. Kernels for blur operations are
            // created separately, and their parameters are set with setArg
            // functions, similar to the C library API.
            if (use_local_mem) blur.dual_pass_local_memory_exchange_box_blur();

            // This example uses a program built with parameters. In the blur.cl
            // file you can find 'USE_SUBGROUP_EXCHANGE_RELATIVE' C-like
            // definition switch for blur_box_horizontal_subgroup_exchange
            // function. In this case, 2 blur kernel functors are used.
            if (use_subgroup_exchange_relative)
            {
                std::cout << "Dual-pass subgroup relative exchange blur"
                          << std::endl;

                blur.build_program("-D USE_SUBGROUP_EXCHANGE_RELATIVE ");
                blur.dual_pass_subgroup_exchange_box_blur();
            }

            // Same as the previous one, but with a different build switch. See
            // the blur.cl file for more info about the switch.
            if (use_subgroup_exchange)
            {
                std::cout << "Dual-pass subgroup exchange blur" << std::endl;

                blur.build_program("-D USE_SUBGROUP_EXCHANGE ");
                blur.dual_pass_subgroup_exchange_box_blur();
            }
        } // Box blur

        // Build default program with no kernel arguments.
        blur.build_program("");

        // The gauss blur operation is performed when the "-b gauss" option or
        // no option is passed. The following examples use a manually created
        // gaussian kernel passed as an argument to functions from blur.cl
        if (blur.option_active("gauss"))
        {
            std::cout << "Dual-pass Gaussian blur" << std::endl;
            // Create a gaussian kernel to be used for the next blurs.
            blur.create_gaussian_kernel();

            // Basic blur operation using kernel functor and gaussian kernel.
            blur.dual_pass_kernel_blur();

            // Local memory exchange Gaussian blur with kernel functors. Note
            // that the variable type cl::Local is used for local memory
            // arguments in kernel functor calls.
            if (use_local_mem)
            {
                std::cout << "Dual-pass local memory exchange Gaussian blur"
                          << std::endl;
                blur.dual_pass_local_memory_exchange_kernel_blur();
            }

            // Similar to dual_pass_subgroup_exchange_box_blur but with a gauss
            // kernel.
            if (use_subgroup_exchange_relative)
            {
                std::cout
                    << "Dual-pass subgroup relative exchange Gaussian blur"
                    << std::endl;

                blur.build_program("-D USE_SUBGROUP_EXCHANGE_RELATIVE ");
                blur.dual_pass_subgroup_exchange_kernel_blur();
            }

            // Same as the previous one, but with a different build switch. See
            // the blur.cl file for more info about the switch.
            if (use_subgroup_exchange)
            {
                std::cout << "Dual-pass subgroup exchange Gaussian blur"
                          << std::endl;

                blur.build_program("-D USE_SUBGROUP_EXCHANGE ");
                blur.dual_pass_subgroup_exchange_kernel_blur();
            }
        } // Gaussian blur
    } catch (cl::Error& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return 0;
}
