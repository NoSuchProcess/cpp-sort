/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Morwenn
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef CPPSORT_ADAPTERS_HYBRID_ADAPTER_H_
#define CPPSORT_ADAPTERS_HYBRID_ADAPTER_H_

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <cpp-sort/sorter_base.h>
#include <cpp-sort/sorter_traits.h>
#include <cpp-sort/utility/any_all.h>

namespace cppsort
{
    namespace detail
    {
        // Here goes what we can't put into hybrid_sorter's privates
        // because full template specialization is not allowed in
        // classes...

        ////////////////////////////////////////////////////////////
        // Overload resolution tool

        template<std::size_t Value>
        struct choice:
            choice<Value + 1>
        {};

        template<>
        struct choice<256> {};

        ////////////////////////////////////////////////////////////
        // Associate a priority to iterator categories, there is
        // probably a trick to automate that...

        template<typename>
        constexpr std::size_t iterator_category_value;

        template<>
        constexpr std::size_t iterator_category_value<std::random_access_iterator_tag> = 0;

        template<>
        constexpr std::size_t iterator_category_value<std::bidirectional_iterator_tag> = 1;

        template<>
        constexpr std::size_t iterator_category_value<std::forward_iterator_tag> = 2;

        template<>
        constexpr std::size_t iterator_category_value<std::input_iterator_tag> = 3;
    }

    ////////////////////////////////////////////////////////////
    // Adapter

    template<typename... Sorters>
    class hybrid_adapter:
        sorter_base<hybrid_adapter<Sorters...>>
    {
        private:

            // Number of acceptable iterator categories
            static constexpr std::size_t categories_number = 4;

            ////////////////////////////////////////////////////////////
            // Import every operator() in one class

            template<typename Head, typename... Tail>
            struct sorters_merger:
                Head, sorters_merger<Tail...>
            {
                using Head::operator();
                using sorters_merger<Tail...>::operator();
            };

            template<typename Head>
            struct sorters_merger<Head>:
                Head
            {
                using Head::operator();
            };

            ////////////////////////////////////////////////////////////
            // Add a dispatch to the operator() so that a sorter is
            // preferred for its iterator category first, then for its
            // position into the sorters

            template<typename Sorter, std::size_t Ind>
            struct selection_wrapper
            {
                template<typename... Args>
                auto operator()(detail::choice<Ind>, Args&&... args) const
                    -> decltype(Sorter{}(std::forward<Args>(args)...))
                {
                    return Sorter{}(std::forward<Args>(args)...);
                }
            };

            // Associate and index to every sorter depending on
            // its position in the parameter pack
            template<typename>
            struct dispatch_sorter_impl;

            template<std::size_t... Indices>
            struct dispatch_sorter_impl<std::index_sequence<Indices...>>
            {
                using type = sorters_merger<
                    selection_wrapper<
                        Sorters,
                        Indices + detail::iterator_category_value<iterator_category<Sorters>>
                                * categories_number
                    >...
                >;
            };

            // Dispatch-enabled sorter
            using dispatch_sorter = typename dispatch_sorter_impl<
                std::make_index_sequence<sizeof...(Sorters)>
            >::type;

        public:

            template<typename Iterable, typename... Args>
            auto operator()(Iterable& iterable, Args&&... args) const
                -> decltype(auto)
            {
                // Iterator category of the iterable to sort
                using category =
                    typename std::iterator_traits<
                        decltype(std::begin(iterable))
                    >::iterator_category;

                // Call the appropriate operator()
                return dispatch_sorter{}(
                    detail::choice<detail::iterator_category_value<category> * categories_number>{},
                    iterable, std::forward<Args>(args)...
                );
            }

            template<typename Iterator, typename... Args>
            auto operator()(Iterator first, Iterator last, Args&&... args) const
                -> decltype(auto)
            {
                // Iterator category of the iterable to sort
                using category = typename std::iterator_traits<Iterator>::iterator_category;

                // Call the appropriate operator()
                return dispatch_sorter{}(
                    detail::choice<detail::iterator_category_value<category> * categories_number>{},
                    first, last, std::forward<Args>(args)...
                );
            }
    };

    // We unwrap nested hybrid_adapter types to improve
    // the accuracy of the iterator category

    template<typename... Sorters1, typename... Sorters2>
    class hybrid_adapter<hybrid_adapter<Sorters1...>, Sorters2...>:
        public hybrid_adapter<Sorters1..., Sorters2...>
    {};

    ////////////////////////////////////////////////////////////
    // Sorter traits

    template<typename... Sorters>
    struct sorter_traits<hybrid_adapter<Sorters...>>
    {
        // The iterator category is the least constrained one
        // among the aggregated sorters
        using iterator_category = std::common_type_t<iterator_category<Sorters>...>;

        // The adapter is stable only if every aggregated sorter
        // is stable
        static constexpr bool is_stable = utility::all(sorter_traits<Sorters>::is_stable...);
    };
}

#endif // CPPSORT_ADAPTERS_HYBRID_ADAPTER_H_
