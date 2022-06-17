//  Copyright (c)      2013 Thomas Heller
//  Copyright (c) 2007-2019 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/schedulers/local_priority_queue_scheduler.hpp>

#include <string>

#include <pika/config/warnings_prefix.hpp>

namespace pika::threads::policies {
    /// The static_priority_queue_scheduler maintains exactly one queue of work
    /// items (threads) per OS thread, where this OS thread pulls its next work
    /// from. Additionally it maintains separate queues: several for high
    /// priority threads and one for low priority threads.
    /// High priority threads are executed by the first N OS threads before any
    /// other work is executed. Low priority threads are executed by the last
    /// OS thread whenever no other work is available.
    /// This scheduler does not do any work stealing.
    class PIKA_EXPORT static_priority_queue_scheduler final
      : public local_priority_queue_scheduler
    {
    public:
        using base_type = local_priority_queue_scheduler;
        using init_parameter_type = typename base_type::init_parameter_type;

        static_priority_queue_scheduler(init_parameter_type const& init,
            bool deferred_initialization = true);
        void set_scheduler_mode(scheduler_mode mode) override;
        static std::string get_scheduler_name();
    };
}    // namespace pika::threads::policies

#include <pika/config/warnings_suffix.hpp>
