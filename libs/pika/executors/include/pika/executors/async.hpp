//  Copyright (c) 2007-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/async_base/async.hpp>
#include <pika/async_base/launch_policy.hpp>
#include <pika/async_base/traits/is_launch_policy.hpp>
#include <pika/execution/detail/async_launch_policy_dispatch.hpp>
#include <pika/execution/executors/execution.hpp>
#include <pika/execution_base/traits/is_executor.hpp>
#include <pika/functional/deferred_call.hpp>
#include <pika/futures/future.hpp>

#include <type_traits>
#include <utility>

namespace pika { namespace detail {
    template <typename Func, typename Enable = void>
    struct async_dispatch_launch_policy_helper;

    template <typename Func>
    struct async_dispatch_launch_policy_helper<Func,
        std::enable_if_t<!traits::is_action_v<Func>>>
    {
        template <typename Policy_, typename F, typename... Ts>
        PIKA_FORCEINLINE static auto call(
            Policy_&& launch_policy, F&& f, Ts&&... ts)
            -> decltype(async_launch_policy_dispatch<std::decay_t<F>>::call(
                PIKA_FORWARD(Policy_, launch_policy), PIKA_FORWARD(F, f),
                PIKA_FORWARD(Ts, ts)...))
        {
            return async_launch_policy_dispatch<std::decay_t<F>>::call(
                PIKA_FORWARD(Policy_, launch_policy), PIKA_FORWARD(F, f),
                PIKA_FORWARD(Ts, ts)...);
        }
    };

    template <typename Policy>
    struct async_dispatch<Policy, std::enable_if_t<is_launch_policy_v<Policy>>>
    {
        template <typename Policy_, typename F, typename... Ts>
        PIKA_FORCEINLINE static auto call(
            Policy_&& launch_policy, F&& f, Ts&&... ts)
            -> decltype(
                async_dispatch_launch_policy_helper<std::decay_t<F>>::call(
                    PIKA_FORWARD(Policy_, launch_policy), PIKA_FORWARD(F, f),
                    PIKA_FORWARD(Ts, ts)...))
        {
            return async_dispatch_launch_policy_helper<std::decay_t<F>>::call(
                PIKA_FORWARD(Policy_, launch_policy), PIKA_FORWARD(F, f),
                PIKA_FORWARD(Ts, ts)...);
        }
    };

    // Launch the given function or function object asynchronously and return a
    // future allowing to synchronize with the returned result.
    template <typename Func, typename Enable>
    struct async_dispatch
    {
        template <typename F, typename... Ts>
        PIKA_FORCEINLINE static std::enable_if_t<
            traits::detail::is_deferred_invocable_v<F, Ts...>,
            pika::future<util::detail::invoke_deferred_result_t<F, Ts...>>>
        call(F&& f, Ts&&... ts)
        {
            return async_launch_policy_dispatch<std::decay_t<F>>::call(
                pika::launch::async, PIKA_FORWARD(F, f),
                PIKA_FORWARD(Ts, ts)...);
        }
    };

    // The overload for pika::async taking an executor simply forwards to the
    // corresponding executor customization point.
    //
    // parallel::execution::executor
    // threads::executor
    template <typename Executor>
    struct async_dispatch<Executor,
        std::enable_if_t<traits::is_one_way_executor_v<Executor> ||
            traits::is_two_way_executor_v<Executor>>>
    {
        template <typename Executor_, typename F, typename... Ts>
        PIKA_FORCEINLINE static decltype(auto) call(
            Executor_&& exec, F&& f, Ts&&... ts)
        {
            return parallel::execution::async_execute(
                PIKA_FORWARD(Executor_, exec), PIKA_FORWARD(F, f),
                PIKA_FORWARD(Ts, ts)...);
        }
    };
}}    // namespace pika::detail
