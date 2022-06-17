//  Copyright (c) 2007-2017 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/modules/logging.hpp>
#include <pika/schedulers/deadlock_detection.hpp>
#include <pika/schedulers/static_queue_scheduler.hpp>
#include <pika/schedulers/thread_queue.hpp>
#include <pika/threading_base/threading_base_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

#include <pika/config/warnings_prefix.hpp>

namespace pika::threads::policies {
    static_queue_scheduler::static_queue_scheduler(
        typename base_type::init_parameter_type const& init,
        bool deferred_initialization)
      : base_type(init, deferred_initialization)
    {
    }

    std::string static_queue_scheduler::get_scheduler_name()
    {
        return "static_queue_scheduler";
    }

    void static_queue_scheduler::set_scheduler_mode(scheduler_mode mode)
    {
        // this scheduler does not support stealing or numa stealing
        mode = scheduler_mode(mode & ~scheduler_mode::enable_stealing);
        mode = scheduler_mode(mode & ~scheduler_mode::enable_stealing_numa);
        scheduler_base::set_scheduler_mode(mode);
    }

    bool static_queue_scheduler::get_next_thread(std::size_t num_thread, bool,
        threads::detail::thread_id_ref_type& thrd, bool /*enable_stealing*/)
    {
        using thread_queue_type = typename base_type::thread_queue_type;

        {
            PIKA_ASSERT(num_thread < this->queues_.size());

            thread_queue_type* q = this->queues_[num_thread];
            bool result = q->get_next_thread(thrd);

            q->increment_num_pending_accesses();
            if (result)
                return true;
            q->increment_num_pending_misses();
        }

        return false;
    }

    bool static_queue_scheduler::wait_or_add_new(std::size_t num_thread,
        bool running, std::int64_t& idle_loop_count, bool /*enable_stealing*/,
        std::size_t& added)
    {
        PIKA_ASSERT(num_thread < this->queues_.size());

        added = 0;

        bool result = true;

        result = this->queues_[num_thread]->wait_or_add_new(running, added) &&
            result;
        if (0 != added)
            return result;

        // Check if we have been disabled
        if (!running)
        {
            return true;
        }

#ifdef PIKA_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
        // no new work is available, are we deadlocked?
        if (PIKA_UNLIKELY(get_minimal_deadlock_detection_enabled() &&
                LPIKA_ENABLED(error)))
        {
            bool suspended_only = true;

            for (std::size_t i = 0; suspended_only && i != this->queues_.size();
                 ++i)
            {
                suspended_only = this->queues_[i]->dump_suspended_threads(
                    i, idle_loop_count, running);
            }

            if (PIKA_UNLIKELY(suspended_only))
            {
                LTM_(warning).format(
                    "queue({}): no new work available, are we deadlocked?",
                    num_thread);
            }
        }
#else
        PIKA_UNUSED(idle_loop_count);
#endif

        return result;
    }
}    // namespace pika::threads::policies

#include <pika/config/warnings_suffix.hpp>
