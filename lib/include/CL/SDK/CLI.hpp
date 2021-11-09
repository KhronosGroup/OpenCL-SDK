#pragma once

// OpenCL SDK includes
#include <CL/SDK/Options.hpp>

// OpenCL Utils includes
#include <CL/Utils/Detail.hpp>

// TCLAP includes
#include <tclap/CmdLine.h>

// STL includes
#include <memory>   // std::shared_ptr, std::make_shared
#include <tuple>    // std::make_tuple
#include <vector>   // std::vector

namespace cl
{
namespace sdk
{
    template <typename Option>
    auto parse();

    template <typename Option, typename... Parsers>
    Option comprehend(Parsers... parsers);

namespace detail
{
    template <typename Option, typename... Types>
    auto comprehend_helper(std::tuple<Types...> parser)
    {
        return util::detail::apply(comprehend<Option, Types...>, parser);
    }
}

    template <typename... Options>
    std::tuple<Options...> parse_cli(int argc, char* argv[], std::string banner = "OpenCL SDK sample template")
    {
        TCLAP::CmdLine cli(banner);

        auto parsers = std::make_tuple(
            std::make_pair(Options{}, parse<Options>())...
        );

        util::detail::for_each_in_tuple(parsers, [&](auto&& parser){
            util::detail::for_each_in_tuple(parser.second, [&](auto&& arg){
                cli.add(arg.get());
            });
        });

        cli.parse(argc, argv);

        return util::detail::transform_tuple(parsers, [](auto&& parser){
            return detail::comprehend_helper<std::remove_reference_t<decltype(parser.first)>>(parser.second);
        });
    }
}
}

// SDK built-in CLI parsers

template <>
auto cl::sdk::parse<cl::sdk::options::Diagnostic>()
{
    return std::make_tuple(
        std::make_shared<TCLAP::SwitchArg>("v", "verbose", "Extra informational output", false),
        std::make_shared<TCLAP::SwitchArg>("q", "quiet", "Suppress standard output", false)
    );
}
template <>
cl::sdk::options::Diagnostic cl::sdk::comprehend<cl::sdk::options::Diagnostic>(
    std::shared_ptr<TCLAP::SwitchArg> verbose_arg,
    std::shared_ptr<TCLAP::SwitchArg> quiet_arg)
{
    return options::Diagnostic{
        verbose_arg->getValue(),
        quiet_arg->getValue()
    };
}

template <>
auto cl::sdk::parse<cl::sdk::options::SingleDevice>()
{
    std::vector<std::string> valid_dev_strings{ "all", "cpu", "gpu", "acc", "cus", "def" };
    TCLAP::ValuesConstraint<std::string> valid_dev_constraint{ valid_dev_strings };

    return std::make_tuple(
        std::make_shared<TCLAP::ValueArg<int>>("p", "platform", "Index of platform to use", false, 0, "positive integral"),
        std::make_shared<TCLAP::ValueArg<int>>("d", "device", "Index of device to use", false, 0, "positive integral"),
        std::make_shared<TCLAP::ValueArg<std::string>>( "t", "type","Type of device to use", false, "def", &valid_dev_constraint)
    );
}
template <>
cl::sdk::options::SingleDevice cl::sdk::comprehend<cl::sdk::options::SingleDevice>(
    std::shared_ptr<TCLAP::ValueArg<int>> platform_arg,
    std::shared_ptr<TCLAP::ValueArg<int>> device_arg,
    std::shared_ptr<TCLAP::ValueArg<std::string>> type_arg)
{
    auto device_type = [](std::string in) -> cl_device_type
    {
        if (in == "all") return CL_DEVICE_TYPE_ALL;
        else if (in == "cpu") return CL_DEVICE_TYPE_CPU;
        else if (in == "gpu") return CL_DEVICE_TYPE_GPU;
        else if (in == "acc") return CL_DEVICE_TYPE_ACCELERATOR;
        else if (in == "cus") return CL_DEVICE_TYPE_CUSTOM;
        else if (in == "def") return CL_DEVICE_TYPE_DEFAULT;
        else throw std::logic_error{ "Unkown device type after cli parse. Should not have happened." };
    };

    return options::SingleDevice{
        platform_arg->getValue(),
        device_arg->getValue(),
        device_type(type_arg->getValue())
    };
}
