#pragma once

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
}
}
}
