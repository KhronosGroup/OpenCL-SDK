#pragma once

// OpenCL SDK includes
#include "OpenCLSDKCpp_Export.h"
#include <CL/SDK/Options.hpp>

// OpenCL Utils includes
#include <CL/Utils/Detail.hpp>

// TCLAP includes
#include <tclap/CmdLine.h>

// STL includes
#include <memory> // std::shared_ptr, std::make_shared
#include <tuple> // std::make_tuple
#include <vector> // std::vector

namespace cl {
namespace sdk {
    template <typename Option> auto parse();

    template <typename Option, typename... Parsers>
    Option comprehend(Parsers... parsers);

    namespace detail {
        template <typename Option, typename... Types>
        auto comprehend_helper(std::tuple<Types...> parser)
        {
            return util::detail::apply(comprehend<Option, Types...>, parser);
        }
    }

    template <typename... Options>
    std::tuple<Options...>
    parse_cli(int argc, char* argv[],
              std::string banner = "OpenCL SDK sample template")
    {
        TCLAP::CmdLine cli(banner);

        auto parsers =
            std::make_tuple(std::make_pair(Options{}, parse<Options>())...);

        util::detail::for_each_in_tuple(parsers, [&](auto&& parser) {
            util::detail::for_each_in_tuple(
                parser.second, [&](auto&& arg) { cli.add(arg.get()); });
        });

        cli.parse(argc, argv);

        return util::detail::transform_tuple(parsers, [](auto&& parser) {
            return detail::comprehend_helper<
                std::remove_reference_t<decltype(parser.first)>>(parser.second);
        });
    }
}
}

// SDK built-in CLI parsers

SDKCPP_EXPORT extern std::unique_ptr<TCLAP::ValuesConstraint<std::string>>
    valid_dev_constraint;

namespace cl {
namespace sdk {

    template <> inline auto parse<options::Diagnostic>()
    {
        return std::make_tuple(
            std::make_shared<TCLAP::SwitchArg>(
                "v", "verbose", "Extra informational output", false),
            std::make_shared<TCLAP::SwitchArg>(
                "q", "quiet", "Suppress standard output", false));
    }
    template <>
    inline options::Diagnostic comprehend<options::Diagnostic>(
        std::shared_ptr<TCLAP::SwitchArg> verbose_arg,
        std::shared_ptr<TCLAP::SwitchArg> quiet_arg)
    {
        return options::Diagnostic{ verbose_arg->getValue(),
                                    quiet_arg->getValue() };
    }

    template <> inline auto parse<options::SingleDevice>()
    {
        std::vector<std::string> valid_dev_strings{ "all", "cpu", "gpu",
                                                    "acc", "cus", "def" };
        valid_dev_constraint =
            std::make_unique<TCLAP::ValuesConstraint<std::string>>(
                valid_dev_strings);

        return std::make_tuple(std::make_shared<TCLAP::ValueArg<unsigned int>>(
                                   "p", "platform", "Index of platform to use",
                                   false, 0, "positive integral"),
                               std::make_shared<TCLAP::ValueArg<unsigned int>>(
                                   "d", "device", "Index of device to use",
                                   false, 0, "positive integral"),
                               std::make_shared<TCLAP::ValueArg<std::string>>(
                                   "t", "type", "Type of device to use", false,
                                   "def", valid_dev_constraint.get()));
    }
    template <>
    inline options::SingleDevice comprehend<options::SingleDevice>(
        std::shared_ptr<TCLAP::ValueArg<unsigned int>> platform_arg,
        std::shared_ptr<TCLAP::ValueArg<unsigned int>> device_arg,
        std::shared_ptr<TCLAP::ValueArg<std::string>> type_arg)
    {
        auto device_type = [](std::string in) -> cl_device_type {
            if (in == "all")
                return CL_DEVICE_TYPE_ALL;
            else if (in == "cpu")
                return CL_DEVICE_TYPE_CPU;
            else if (in == "gpu")
                return CL_DEVICE_TYPE_GPU;
            else if (in == "acc")
                return CL_DEVICE_TYPE_ACCELERATOR;
            else if (in == "cus")
                return CL_DEVICE_TYPE_CUSTOM;
            else if (in == "def")
                return CL_DEVICE_TYPE_DEFAULT;
            else
                throw std::logic_error{ "Unkown device type after cli parse." };
        };

        return options::SingleDevice{ platform_arg->getValue(),
                                      device_arg->getValue(),
                                      device_type(type_arg->getValue()) };
    }

    template <> inline auto parse<options::Window>()
    {
        return std::make_tuple(
            std::make_shared<TCLAP::ValueArg<int>>("x", "width",
                                                   "Width of window", false,
                                                   800, "positive integral"),
            std::make_shared<TCLAP::ValueArg<int>>("y", "height",
                                                   "Height of window", false,
                                                   800, "positive integral"),
            std::make_shared<TCLAP::SwitchArg>("f", "fullscreen",
                                               "Fullscreen window", false));
    }
    template <>
    inline options::Window comprehend<options::Window>(
        std::shared_ptr<TCLAP::ValueArg<int>> width_arg,
        std::shared_ptr<TCLAP::ValueArg<int>> height_arg,
        std::shared_ptr<TCLAP::SwitchArg> fullscreen_arg)
    {
        return options::Window{ width_arg->getValue(), height_arg->getValue(),
                                fullscreen_arg->getValue() };
    }

}
}
