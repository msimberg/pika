//  Copyright (c)      2013 Thomas Heller
//  Copyright (c) 2007-2019 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/schedulers/static_priority_queue_scheduler.hpp>

#include <string>

#include <pika/config/warnings_prefix.hpp>

namespace pika::threads::policies {
    static_priority_queue_scheduler::static_priority_queue_scheduler(
        init_parameter_type const& init, bool deferred_initialization)
      : base_type(init, deferred_initialization)
    {
        // disable thread stealing to begin with
        this->remove_scheduler_mode(scheduler_mode(
            policies::enable_stealing | policies::enable_stealing_numa));
    }

    void static_priority_queue_scheduler::set_scheduler_mode(
        scheduler_mode mode)
    {
        // this scheduler does not support stealing or numa stealing
        mode = scheduler_mode(mode & ~scheduler_mode::enable_stealing);
        mode = scheduler_mode(mode & ~scheduler_mode::enable_stealing_numa);
        scheduler_base::set_scheduler_mode(mode);
    }

    std::string static_priority_queue_scheduler::get_scheduler_name()
    {
        return "static_priority_queue_scheduler";
    }
}    // namespace pika::threads::policies

#include <pika/config/warnings_suffix.hpp>
