//  Copyright (c) 2007-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/local/config.hpp>
#include <pika/async_base/sync.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace pika {
    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        // dispatch point used for sync<Action> implementations
        template <typename Action, typename Func, typename Enable = void>
        struct sync_action_dispatch;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename F, typename... Ts>
    PIKA_FORCEINLINE auto sync(F&& f, Ts&&... ts)
        -> decltype(detail::sync_action_dispatch<Action, std::decay_t<F>>::call(
            PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...));
}    // namespace pika
