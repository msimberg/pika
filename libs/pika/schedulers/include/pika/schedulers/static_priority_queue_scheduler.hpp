//  Copyright (c)      2013 Thomas Heller
//  Copyright (c) 2007-2019 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/schedulers/local_priority_queue_scheduler.hpp>
#include <pika/schedulers/lockfree_queue_backends.hpp>

#if !defined(PIKA_HAVE_MODULE)
#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#endif

#include <pika/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace pika::threads::detail {
    ///////////////////////////////////////////////////////////////////////////
    /// The static_priority_queue_scheduler maintains exactly one queue of work
    /// items (threads) per OS thread, where this OS thread pulls its next work
    /// from. Additionally it maintains separate queues: several for high
    /// priority threads and one for low priority threads.
    /// High priority threads are executed by the first N OS threads before any
    /// other work is executed. Low priority threads are executed by the last
    /// OS thread whenever no other work is available.
    /// This scheduler does not do any work stealing.
    template <typename Mutex = std::mutex, typename PendingQueuing = lockfree_fifo,
        typename StagedQueuing = lockfree_fifo, typename TerminatedQueuing = lockfree_fifo>
    class PIKA_EXPORT static_priority_queue_scheduler
      : public local_priority_queue_scheduler<Mutex, PendingQueuing, StagedQueuing,
            TerminatedQueuing>
    {
    public:
        using base_type =
            local_priority_queue_scheduler<Mutex, PendingQueuing, StagedQueuing, TerminatedQueuing>;

        using init_parameter_type = typename base_type::init_parameter_type;

        static_priority_queue_scheduler(
            init_parameter_type const& init, bool deferred_initialization = true)
          : base_type(init, deferred_initialization)
        {
            // disable thread stealing to begin with
            this->remove_scheduler_mode(scheduler_mode(
                scheduler_mode::enable_stealing | scheduler_mode::enable_stealing_numa));
        }

        void set_scheduler_mode(scheduler_mode mode) override
        {
            // this scheduler does not support stealing or numa stealing
            mode = scheduler_mode(mode & ~scheduler_mode::enable_stealing);
            mode = scheduler_mode(mode & ~scheduler_mode::enable_stealing_numa);
            scheduler_base::set_scheduler_mode(mode);
        }

        static std::string get_scheduler_name() { return "static_priority_queue_scheduler"; }
    };
}    // namespace pika::threads::detail

template <typename Mutex, typename PendingQueuing, typename StagedQueuing,
    typename TerminatedQueuing>
struct fmt::formatter<pika::threads::detail::static_priority_queue_scheduler<Mutex, PendingQueuing,
    StagedQueuing, TerminatedQueuing>> : fmt::formatter<pika::threads::detail::scheduler_base>
{
    template <typename FormatContext>
    auto format(pika::threads::detail::scheduler_base const& scheduler, FormatContext& ctx) const
    {
        return fmt::formatter<pika::threads::detail::scheduler_base>::format(scheduler, ctx);
    }
};

#include <pika/config/warnings_suffix.hpp>
