//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/execution_base/receiver.hpp>
#include <pika/type_support/pack.hpp>

#include <type_traits>
#include <variant>

namespace pika::execution::experimental::detail {

    template <typename T>
    struct result_type_signature_helper
    {
        using type = pika::execution::experimental::set_value_t(T);
    };

    template <>
    struct result_type_signature_helper<void>
    {
        using type = pika::execution::experimental::set_value_t();
    };

    template <typename T>
    using result_type_signature_helper_t = typename result_type_signature_helper<T>::type;

    template <typename Variants>
    struct single_result
    {
        static_assert(sizeof(Variants) == 0,
            "expected a single variant with a single type in "
            "sender_traits<>::value_types");
    };

    template <>
    struct single_result<pika::util::detail::pack<pika::util::detail::pack<>>>
    {
        using type = void;
    };

    template <typename T>
    struct single_result<pika::util::detail::pack<pika::util::detail::pack<T>>>
    {
        using type = T;
    };

    template <typename T, typename U, typename... Ts>
    struct single_result<pika::util::detail::pack<pika::util::detail::pack<T, U, Ts...>>>
    {
        static_assert(sizeof(T) == 0,
            "expected a single variant with a single type in "
            "sender_traits<>::value_types (single variant with two or more "
            "types given)");
    };

    template <typename T, typename U, typename... Ts>
    struct single_result<pika::util::detail::pack<T, U, Ts...>>
    {
        static_assert(sizeof(T) == 0,
            "expected a single variant with a single type in "
            "sender_traits<>::value_types (two or more variants)");
    };

    template <typename Variants>
    using single_result_t = typename single_result<Variants>::type;

    template <typename Variants>
    struct single_result_non_void
    {
        using type = typename single_result<Variants>::type;
        static_assert(!std::is_void<type>::value, "expected a non-void type in single_result");
    };

    template <typename Variants>
    using single_result_non_void_t = typename single_result_non_void<Variants>::type;

    template <typename Variants>
    struct single_variant
    {
        static_assert(
            sizeof(Variants) == 0, "expected a single variant sender_traits<>::value_types");
    };

    template <typename T>
    struct single_variant<pika::util::detail::pack<T>>
    {
        using type = T;
    };

    template <typename Variants>
    using single_variant_t = typename single_variant<Variants>::type;

}    // namespace pika::execution::experimental::detail
