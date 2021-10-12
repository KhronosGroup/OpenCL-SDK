#pragma once

#include <tuple>

namespace cl
{
namespace sdk
{
namespace detail
{
    // Borrowed from: https://www.fluentcpp.com/2019/03/05/for_each_arg-applying-a-function-to-each-argument-of-a-function-in-cpp/
    template<class F, class...Args>
    F for_each_arg(F f, Args&&...args)
    {
        std::initializer_list<int>{((void)f(std::forward<Args>(args)), 0)...};
        return f;
    }

    // Helpers for for_each_elem
    template<typename T, typename F, int... Is>
    void
    for_each(T&& t, F f, std::integer_sequence<int, Is...>)
    {
        auto l = { (f(std::get<Is>(t)), 0)... };
    }

    template<typename... Ts, typename F>
    void
    for_each_in_tuple(std::tuple<Ts...> const& t, F f)
    {
        detail::for_each(t, f, std::make_integer_sequence<int, sizeof...(Ts)>());
    }

    template <class F, typename Tuple, size_t... Is>
    auto transform_each_impl(Tuple t, F f, std::index_sequence<Is...>) {
        return std::make_tuple(
            f(std::get<Is>(t) )...
        );
    }

    template <class F, typename... Args>
    auto transform_each(const std::tuple<Args...>& t, F f) {
        return detail::transform_each_impl(
            t, f, std::make_index_sequence<sizeof...(Args)>{});
    }
}
}
}
