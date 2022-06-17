//  Copyright (c) 2007-2017 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/schedulers/local_queue_scheduler.hpp>
#include <pika/threading_base/threading_base_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

#include <pika/config/warnings_prefix.hpp>

namespace pika::threads::policies {
    /// The local_queue_scheduler maintains exactly one queue of work
    /// items (threads) per OS thread, where this OS thread pulls its next work
    /// from.
    class PIKA_EXPORT static_queue_scheduler final
      : public local_queue_scheduler
    {
    public:
        using base_type = local_queue_scheduler;

        static_queue_scheduler(
            typename base_type::init_parameter_type const& init,
            bool deferred_initialization = true);
        static std::string get_scheduler_name();

        void set_scheduler_mode(scheduler_mode mode) override;

        /// Return the next thread to be executed, return false if none is
        /// available
        bool get_next_thread(std::size_t num_thread, bool,
            threads::detail::thread_id_ref_type& thrd,
            bool /*enable_stealing*/) override;

        /// This is a function which gets called periodically by the thread
        /// manager to allow for maintenance tasks to be executed in the
        /// scheduler. Returns true if the OS thread calling this function
        /// has to be terminated (i.e. no more work has to be done).
        bool wait_or_add_new(std::size_t num_thread, bool running,
            std::int64_t& idle_loop_count, bool /*enable_stealing*/,
            std::size_t& added) override;
    };
}    // namespace pika::threads::policies

#include <pika/config/warnings_suffix.hpp>
