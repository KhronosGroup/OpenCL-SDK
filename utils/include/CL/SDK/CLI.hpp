#pragma once

#include <CL/SDK/Detail.hpp>
#include <CL/SDK/Options.hpp>
#include <CL/SDK/Error.hpp>

namespace cl
{
namespace sdk
{
    template <typename Option>
    Option parse(int argc, char* argv[]);

    template <typename... Options>
    std::tuple<Options...> parse_cli(int argc, char* argv[])
    {
        return std::make_tuple(
            parse<Options>(argc, argv)...
        );
    }
}
}

// TCLAP includes
#include <tclap/CmdLine.h>

template <>
cl::sdk::options::Diagnostic cl::sdk::parse<cl::sdk::options::Diagnostic>(int argc, char* argv[])
{
    TCLAP::CmdLine cli("");

    TCLAP::SwitchArg verbose_arg("v", "verbose", "Extra informational output", false);
    TCLAP::SwitchArg quiet_arg("q", "quiet", "Suppress standard output", false);

    cli.add(verbose_arg);
    cli.add(quiet_arg);

    cli.parse(argc, argv);

    return options::Diagnostic{
        verbose_arg.getValue(),
        quiet_arg.getValue()
    };
}

template <>
cl::sdk::options::SingleDevice cl::sdk::parse<cl::sdk::options::SingleDevice>(int argc, char* argv[])
{
    TCLAP::CmdLine cli("");

    TCLAP::ValueArg<int> platform_arg("p", "platform", "Index of platform to use", false, 0, "positive integral", cli );
    TCLAP::ValueArg<int> device_arg("d", "device", "Index of device to use", false, 0, "positive integral", cli);

    std::vector<std::string> valid_dev_strings{ "all", "cpu", "gpu", "acc", "def" };
    TCLAP::ValuesConstraint<std::string> valid_dev_constraint{ valid_dev_strings };

    TCLAP::ValueArg<std::string> type_arg{ "t", "type","Type of device to use", false, "def", &valid_dev_constraint , cli };

    auto device_type = [](std::string in) -> cl_device_type
    {
        if (in == "all") return CL_DEVICE_TYPE_ALL;
        else if (in == "cpu") return CL_DEVICE_TYPE_CPU;
        else if (in == "gpu") return CL_DEVICE_TYPE_GPU;
        else if (in == "acc") return CL_DEVICE_TYPE_ACCELERATOR;
        else if (in == "def") return CL_DEVICE_TYPE_DEFAULT;
        else throw std::logic_error{ "Unkown device type after cli parse. Should not have happened." };
    };

    cli.parse(argc, argv);

    return options::SingleDevice{
        platform_arg.getValue(),
        device_arg.getValue(),
        device_type(type_arg.getValue())
    };
}
