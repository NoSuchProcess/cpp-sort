// Range v3 library
//
//  Copyright Eric Niebler 2013-2014
//  Modified in 2015-2016 by Morwenn for inclusion into cpp-sort
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//
#ifndef CPPSORT_UTILITY_AS_FUNCTION_H_
#define CPPSORT_UTILITY_AS_FUNCTION_H_

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <functional>
#include <type_traits>
#include <utility>
#include <cpp-sort/utility/static_const.h>

namespace cppsort
{
namespace utility
{
    namespace detail
    {
        struct as_function_fn
        {
            template<typename T>
            constexpr auto operator()(T&& t) const
                noexcept(noexcept(std::mem_fn(t)))
                -> std::enable_if_t<
                    std::is_member_pointer<std::decay_t<T>>::value,
                    decltype(std::mem_fn(t))
                >
            {
                return std::mem_fn(t);
            }

            template<typename T>
            constexpr auto operator()(T && t) const
                noexcept(std::is_nothrow_constructible<T, T>::value)
                -> std::enable_if_t<
                    not std::is_member_pointer<std::decay_t<T>>::value,
                    T
                >
            {
                return std::forward<T>(t);
            }
        };
    }

    namespace
    {
        constexpr auto&& as_function = static_const<
            detail::as_function_fn
        >::value;
    }
}}

#endif // CPPSORT_UTILITY_AS_FUNCTION_H_
