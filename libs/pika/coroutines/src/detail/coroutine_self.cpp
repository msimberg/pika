//  Copyright (c) 2008-2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/coroutines/detail/coroutine_self.hpp>
#include <pika/logging.hpp>

#include <cstddef>

namespace pika::threads::coroutines::detail {
    struct [[nodiscard]] require_no_yield
    {
        PIKA_EXPORT require_no_yield() noexcept;
        require_no_yield(require_no_yield&&) = delete;
        require_no_yield& operator=(require_no_yield&&) = delete;
        require_no_yield(require_no_yield const&) = delete;
        require_no_yield& operator=(require_no_yield const&) = delete;
        PIKA_EXPORT ~require_no_yield() noexcept;
        bool old;
    };

    static thread_local bool do_check_yield = false;

    require_no_yield::require_no_yield() noexcept
      : old(do_check_yield)
    {
        do_check_yield = true;
    }

    require_no_yield::~require_no_yield() noexcept { do_check_yield = old; }

    void check_yield() noexcept
    {
        if (do_check_yield)
        {
            PIKA_LOG(err,
                "trying to yield a pika thread when it has been "
                "disallowed, terminating");
            std::terminate();
        }
    }

    coroutine_self*& coroutine_self::local_self()
    {
        static thread_local coroutine_self* local_self_ = nullptr;
        return local_self_;
    }
}    // namespace pika::threads::coroutines::detail
