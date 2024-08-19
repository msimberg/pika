//  Copyright (c) 2007-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

PIKA_GLOBAL_MODULE_FRAGMENT

#include <pika/config.hpp>

#if !defined(PIKA_HAVE_MODULE)
#include <pika/logging.hpp>
#include <pika/modules/coroutines.hpp>
#include <pika/modules/errors.hpp>
#include <pika/threading_base/create_work.hpp>
#include <pika/threading_base/scheduler_base.hpp>
#include <pika/threading_base/thread_data.hpp>
#include <pika/threading_base/thread_init_data.hpp>
#endif

#if defined(PIKA_HAVE_MODULE)
module pika.threading_base;
#endif

namespace pika::threads::detail {
    thread_id_ref_type create_work(
        scheduler_base* scheduler, thread_init_data& data, error_code& ec)
    {
        // verify parameters
        switch (data.initial_state)
        {
        case thread_schedule_state::pending:
        case thread_schedule_state::pending_do_not_schedule:
        case thread_schedule_state::pending_boost:
        case thread_schedule_state::suspended: break;

        default:
        {
            PIKA_THROWS_IF(ec, pika::error::bad_parameter, "thread::detail::create_work",
                "invalid initial state: {}", data.initial_state);
            return invalid_thread_id;
        }
        }

#ifdef PIKA_HAVE_THREAD_DESCRIPTION
        if (!data.description)
        {
            PIKA_THROWS_IF(ec, pika::error::bad_parameter, "thread::detail::create_work",
                "description is nullptr");
            return invalid_thread_id;
        }
#endif

        PIKA_LOG(info,
            "create_work: pool({}), scheduler({}), initial_state({}), thread_priority({}), "
            "description({})",
            *scheduler->get_parent_pool(), *scheduler, get_thread_state_name(data.initial_state),
            execution::detail::get_thread_priority_name(data.priority), data.get_description());

        thread_self* self = get_self_ptr();

#ifdef PIKA_HAVE_THREAD_PARENT_REFERENCE
        if (nullptr == data.parent_id)
        {
            if (self)
            {
                data.parent_id = get_thread_id_data(self->get_thread_id());
                data.parent_phase = self->get_thread_phase();
            }
        }
#endif

        if (nullptr == data.scheduler_base) data.scheduler_base = scheduler;

        // Pass recursive high priority from parent to child.
        if (self)
        {
            if (data.priority == execution::thread_priority::default_ &&
                execution::thread_priority::high_recursive ==
                    get_thread_id_data(self->get_thread_id())->get_priority())
            {
                data.priority = execution::thread_priority::high_recursive;
            }
        }

        // create the new thread
        if (data.priority == execution::thread_priority::default_)
            data.priority = execution::thread_priority::normal;

        data.run_now = (execution::thread_priority::high == data.priority ||
            execution::thread_priority::high_recursive == data.priority ||
            execution::thread_priority::boost == data.priority);

        thread_id_ref_type id = invalid_thread_id;
        scheduler->create_thread(data, data.run_now ? &id : nullptr, ec);

        // NOTE: Don't care if the hint is a NUMA hint, just want to wake up a
        // thread.
        scheduler->do_some_work(data.schedulehint.hint);

        return id;
    }
}    // namespace pika::threads::detail
