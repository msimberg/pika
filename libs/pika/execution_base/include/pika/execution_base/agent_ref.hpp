//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/local/config.hpp>
#include <pika/timing/steady_clock.hpp>

#include <chrono>
#include <cstddef>
#include <iosfwd>

namespace pika { namespace execution_base {

    struct agent_base;

    class PIKA_EXPORT agent_ref
    {
    public:
        constexpr agent_ref() noexcept
          : impl_(nullptr)
        {
        }
        constexpr agent_ref(agent_base* impl) noexcept
          : impl_(impl)
        {
        }

        constexpr agent_ref(agent_ref const&) noexcept = default;
        constexpr agent_ref& operator=(agent_ref const&) noexcept = default;

        constexpr agent_ref(agent_ref&&) noexcept = default;
        constexpr agent_ref& operator=(agent_ref&&) noexcept = default;

        constexpr explicit operator bool() const noexcept
        {
            return impl_ != nullptr;
        }

        void reset(agent_base* impl = nullptr)
        {
            impl_ = impl;
        }

        void yield(char const* desc = "pika::execution_base::agent_ref::yield");
        void yield_k(std::size_t k,
            char const* desc = "pika::execution_base::agent_ref::yield_k");
        void suspend(
            char const* desc = "pika::execution_base::agent_ref::suspend");
        void resume(
            char const* desc = "pika::execution_base::agent_ref::resume");
        void abort(char const* desc = "pika::execution_base::agent_ref::abort");

        template <typename Rep, typename Period>
        void sleep_for(std::chrono::duration<Rep, Period> const& sleep_duration,
            char const* desc = "pika::execution_base::agent_ref::sleep_for")
        {
            sleep_for(pika::chrono::steady_duration{sleep_duration}, desc);
        }

        template <typename Clock, typename Duration>
        void sleep_until(
            std::chrono::time_point<Clock, Duration> const& sleep_time,
            char const* desc = "pika::execution_base::agent_ref::sleep_until")
        {
            sleep_until(pika::chrono::steady_time_point{sleep_time}, desc);
        }

        agent_base& ref()
        {
            return *impl_;
        }

        // TODO:
        // affinity
        // thread_num
        // executor

    private:
        agent_base* impl_;

        void sleep_for(pika::chrono::steady_duration const& sleep_duration,
            char const* desc);
        void sleep_until(pika::chrono::steady_time_point const& sleep_time,
            char const* desc);

        friend constexpr bool operator==(
            agent_ref const& lhs, agent_ref const& rhs)
        {
            return lhs.impl_ == rhs.impl_;
        }

        friend constexpr bool operator!=(
            agent_ref const& lhs, agent_ref const& rhs)
        {
            return lhs.impl_ != rhs.impl_;
        }

        PIKA_EXPORT friend std::ostream& operator<<(
            std::ostream&, agent_ref const&);
    };
}}    // namespace pika::execution_base
