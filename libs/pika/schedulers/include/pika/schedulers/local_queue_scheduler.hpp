//  Copyright (c) 2007-2017 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/affinity/affinity_data.hpp>
#include <pika/assert.hpp>
#include <pika/errors/error_code.hpp>
#include <pika/functional/function.hpp>
#include <pika/schedulers/thread_queue.hpp>
#include <pika/threading_base/scheduler_base.hpp>
#include <pika/threading_base/thread_data.hpp>
#include <pika/threading_base/thread_queue_init_parameters.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#include <pika/config/warnings_prefix.hpp>

namespace pika::threads::policies {
    /// The local_queue_scheduler maintains exactly one queue of work
    /// items (threads) per OS thread, where this OS thread pulls its next work
    /// from.
    class PIKA_EXPORT local_queue_scheduler : public scheduler_base
    {
    public:
        using has_periodic_maintenance = std::false_type;
        using thread_queue_type = thread_queue;

        struct init_parameter
        {
            init_parameter(std::size_t num_queues,
                pika::detail::affinity_data const& affinity_data,
                thread_queue_init_parameters thread_queue_init = {},
                char const* description = "local_queue_scheduler")
              : num_queues_(num_queues)
              , thread_queue_init_(thread_queue_init)
              , affinity_data_(affinity_data)
              , description_(description)
            {
            }

            init_parameter(std::size_t num_queues,
                pika::detail::affinity_data const& affinity_data,
                char const* description)
              : num_queues_(num_queues)
              , thread_queue_init_()
              , affinity_data_(affinity_data)
              , description_(description)
            {
            }

            std::size_t num_queues_;
            thread_queue_init_parameters thread_queue_init_;
            pika::detail::affinity_data const& affinity_data_;
            char const* description_;
        };
        using init_parameter_type = init_parameter;

        local_queue_scheduler(init_parameter_type const& init,
            bool deferred_initialization = true);
        virtual ~local_queue_scheduler();

        static std::string get_scheduler_name();

#ifdef PIKA_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::uint64_t get_creation_time(bool reset) override;
        std::uint64_t get_cleanup_time(bool reset) override;
#endif

#ifdef PIKA_HAVE_THREAD_STEALING_COUNTS
        std::int64_t get_num_pending_misses(
            std::size_t num_thread, bool reset) override;
        std::int64_t get_num_pending_accesses(
            std::size_t num_thread, bool reset) override;
        std::int64_t get_num_stolen_from_pending(
            std::size_t num_thread, bool reset) override;
        std::int64_t get_num_stolen_to_pending(
            std::size_t num_thread, bool reset) override;
        std::int64_t get_num_stolen_from_staged(
            std::size_t num_thread, bool reset) override;
        std::int64_t get_num_stolen_to_staged(
            std::size_t num_thread, bool reset) override;
#endif

        void abort_all_suspended_threads() override;

        bool cleanup_terminated(bool delete_all) override;
        bool cleanup_terminated(
            std::size_t num_thread, bool delete_all) override;

        // create a new thread and schedule it if the initial state is equal to
        // pending
        void create_thread(threads::detail::thread_init_data& data,
            threads::detail::thread_id_ref_type* id, error_code& ec) override;

        /// Return the next thread to be executed, return false if none is
        /// available
        virtual bool get_next_thread(std::size_t num_thread, bool running,
            threads::detail::thread_id_ref_type& thrd,
            bool /*enable_stealing*/) override;

        /// Schedule the passed thread
        void schedule_thread(threads::detail::thread_id_ref_type thrd,
            execution::thread_schedule_hint schedulehint, bool allow_fallback,
            execution::thread_priority /* priority */ =
                execution::thread_priority::normal) override;

        void schedule_thread_last(threads::detail::thread_id_ref_type thrd,
            execution::thread_schedule_hint schedulehint, bool allow_fallback,
            execution::thread_priority /* priority */ =
                execution::thread_priority::normal) override;

        /// Destroy the passed thread as it has been terminated
        void destroy_thread(threads::detail::thread_data* thrd) override;

        // This returns the current length of the queues (work items and new items)
        std::int64_t get_queue_length(
            std::size_t num_thread = std::size_t(-1)) const override;

        // Queries the current thread count of the queues.
        std::int64_t get_thread_count(
            threads::detail::thread_schedule_state state =
                threads::detail::thread_schedule_state::unknown,
            execution::thread_priority priority =
                execution::thread_priority::default_,
            std::size_t num_thread = std::size_t(-1),
            bool /* reset */ = false) const override;

        // Queries whether a given core is idle
        bool is_core_idle(std::size_t num_thread) const override;

        // Enumerate matching threads from all queues
        bool enumerate_threads(
            util::function<bool(threads::detail::thread_id_type)> const& f,
            threads::detail::thread_schedule_state state =
                threads::detail::thread_schedule_state::unknown) const override;

#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
        // Queries the current average thread wait time of the queues.
        std::int64_t get_average_thread_wait_time(
            std::size_t num_thread = std::size_t(-1)) const override;

        // Queries the current average task wait time of the queues.
        std::int64_t get_average_task_wait_time(
            std::size_t num_thread = std::size_t(-1)) const override;
#endif

        /// This is a function which gets called periodically by the thread
        /// manager to allow for maintenance tasks to be executed in the
        /// scheduler. Returns true if the OS thread calling this function
        /// has to be terminated (i.e. no more work has to be done).
        virtual bool wait_or_add_new(std::size_t num_thread, bool running,
            std::int64_t& idle_loop_count, bool /* enable_stealing */,
            std::size_t& added) override;

        void on_start_thread(std::size_t num_thread) override;
        void on_stop_thread(std::size_t num_thread) override;
        void on_error(
            std::size_t num_thread, std::exception_ptr const& e) override;

    protected:
        std::vector<thread_queue_type*> queues_;
        std::atomic<std::size_t> curr_queue_;

        pika::detail::affinity_data const& affinity_data_;

        // we know that the MIC has one NUMA domain only
#if !defined(PIKA_NATIVE_MIC)
        ::pika::threads::detail::mask_type steals_in_numa_domain_;
        ::pika::threads::detail::mask_type steals_outside_numa_domain_;
#endif
        std::vector<::pika::threads::detail::mask_type> numa_domain_masks_;
        std::vector<::pika::threads::detail::mask_type>
            outside_numa_domain_masks_;
    };
}    // namespace pika::threads::policies

#include <pika/config/warnings_suffix.hpp>
