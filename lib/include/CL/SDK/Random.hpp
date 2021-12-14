#pragma once

// OpenCL SDK includes
#include <CL/Utils/Detail.hpp> // cl::util::detail::for_each_arg

// STL includes
#include <algorithm> // std::generate_n
#include <iterator> // std::begin
#include <valarray>

namespace cl {
namespace sdk {
    template <typename PRNG, typename... Containers>
    void fill_with_random(PRNG&& prng, Containers&&... containers)
    {
        util::detail::for_each_arg(
            [&](auto&& container) {
                std::generate_n(std::begin(container), container.size(), prng);
            },
            containers...);
    }
}
}
