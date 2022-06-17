//  Copyright (c) 2007-2019 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/allocator_support/internal_allocator.hpp>
#include <pika/concurrency/cache_line_data.hpp>
#include <pika/concurrency/concurrentqueue.hpp>
#include <pika/errors/error_code.hpp>
#include <pika/functional/function.hpp>
#include <pika/threading_base/scheduler_base.hpp>
#include <pika/threading_base/thread_data.hpp>
#include <pika/threading_base/thread_queue_init_parameters.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pika::threads::policies {
    class thread_queue
    {
    private:
        using mutex_type = std::mutex;

#if defined(PIKA_HAVE_SCHEDULER_THREAD_MAP)
        // this is the type of a map holding all threads (except depleted ones)
        using thread_map_type = std::unordered_set<
            threads::detail::thread_id_type,
            std::hash<threads::detail::thread_id_type>,
            std::equal_to<threads::detail::thread_id_type>,
            pika::detail::internal_allocator<threads::detail::thread_id_type>>;
#endif

        using thread_heap_type = std::vector<threads::detail::thread_id_type,
            pika::detail::internal_allocator<threads::detail::thread_id_type>>;

        struct task_description
        {
            threads::detail::thread_init_data data;
#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
            std::uint64_t waittime;
#endif
        };

#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
        struct thread_description
        {
            threads::detail::thread_id_ref_type data;
            std::uint64_t waittime;
        };
        using thread_description_ptr = thread_description;
#else
        using thread_description_ptr =
            typename threads::detail::thread_id_ref_type::thread_repr*;
#endif

        using work_items_type =
            pika::concurrency::detail::ConcurrentQueue<thread_description_ptr>;

        using task_items_type =
            pika::concurrency::detail::ConcurrentQueue<task_description>;

        using terminated_items_type =
            pika::concurrency::detail::ConcurrentQueue<
                threads::detail::thread_data*>;

    protected:
        void create_thread_object(threads::detail::thread_id_ref_type& thrd,
            threads::detail::thread_init_data& data,
            std::unique_lock<mutex_type>& lk);
        // add new threads if there is some amount of work available
        std::size_t add_new(std::int64_t add_count, thread_queue* addfrom,
            std::unique_lock<mutex_type>& lk, bool /* steal */ = false);
        bool add_new_always(std::size_t& added, thread_queue* addfrom,
            std::unique_lock<mutex_type>& lk, bool steal = false);
        void recycle_thread(threads::detail::thread_id_type thrd);
        bool cleanup_terminated_locked(bool delete_all = false);

    public:
        // This function makes sure all threads which are marked for deletion
        // (state is terminated) are properly destroyed.
        //
        // This returns 'true' if there are no more terminated threads waiting
        // to be deleted.
        bool cleanup_terminated(bool delete_all = false);

        thread_queue(std::size_t /* queue_num */ = std::size_t(-1),
            thread_queue_init_parameters parameters = {});
        ~thread_queue();
        static void deallocate(threads::detail::thread_data* p);

#ifdef PIKA_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::uint64_t get_creation_time(bool reset);
        std::uint64_t get_cleanup_time(bool reset);
#endif

        // This returns the current length of the queues (work items and new items)
        std::int64_t get_queue_length(
            std::memory_order order = std::memory_order_acquire) const;
        // This returns the current length of the pending queue
        std::int64_t get_pending_queue_length(
            std::memory_order order = std::memory_order_acquire) const;
        // This returns the current length of the staged queue
        std::int64_t get_staged_queue_length(
            std::memory_order order = std::memory_order_acquire) const;

#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
        std::uint64_t get_average_task_wait_time() const;
        std::uint64_t get_average_thread_wait_time() const;
#endif

#ifdef PIKA_HAVE_THREAD_STEALING_COUNTS
        std::int64_t get_num_pending_misses(bool reset);
        void increment_num_pending_misses(std::size_t num = 1);
        std::int64_t get_num_pending_accesses(bool reset);
        void increment_num_pending_accesses(std::size_t num = 1);
        std::int64_t get_num_stolen_from_pending(bool reset);
        void increment_num_stolen_from_pending(std::size_t num = 1);
        std::int64_t get_num_stolen_from_staged(bool reset);
        void increment_num_stolen_from_staged(std::size_t num = 1);
        std::int64_t get_num_stolen_to_pending(bool reset);
        void increment_num_stolen_to_pending(std::size_t num = 1);
        std::int64_t get_num_stolen_to_staged(bool reset);
        void increment_num_stolen_to_staged(std::size_t num = 1);
#else
        void increment_num_pending_misses(std::size_t /* num */ = 1);
        void increment_num_pending_accesses(std::size_t /* num */ = 1);
        void increment_num_stolen_from_pending(std::size_t /* num */ = 1);
        void increment_num_stolen_from_staged(std::size_t /* num */ = 1);
        void increment_num_stolen_to_pending(std::size_t /* num */ = 1);
        void increment_num_stolen_to_staged(std::size_t /* num */ = 1);
#endif

        // create a new thread and schedule it if the initial state is equal to
        // pending
        void create_thread(threads::detail::thread_init_data& data,
            threads::detail::thread_id_ref_type* id, error_code& ec);
        // Return the next thread to be executed, return false if none is
        // available
        bool get_next_thread(threads::detail::thread_id_ref_type& thrd,
            bool allow_stealing = false, bool /* steal */ = false) PIKA_HOT;
        // Schedule the passed thread
        void schedule_thread(threads::detail::thread_id_ref_type thrd,
            bool /* other_end */ = false);
        // Destroy the passed thread as it has been terminated
        void destroy_thread(threads::detail::thread_data* thrd);

        // Return the number of existing threads with the given state.
        std::int64_t get_thread_count(
            threads::detail::thread_schedule_state state =
                threads::detail::thread_schedule_state::unknown) const;

        void abort_all_suspended_threads();
        bool enumerate_threads(
            util::function<bool(threads::detail::thread_id_type)> const& f,
            threads::detail::thread_schedule_state state =
                threads::detail::thread_schedule_state::unknown) const;

        // This is a function which gets called periodically by the thread
        // manager to allow for maintenance tasks to be executed in the
        // scheduler. Returns true if the OS thread calling this function
        // has to be terminated (i.e. no more work has to be done).
        bool wait_or_add_new(
            bool, std::size_t& added, bool steal = false) PIKA_HOT;
        bool wait_or_add_new(bool running, std::size_t& added,
            thread_queue* addfrom, bool steal = false) PIKA_HOT;

        bool dump_suspended_threads(std::size_t num_thread,
            std::int64_t& idle_loop_count, bool running);

        void on_start_thread(std::size_t /* num_thread */);
        void on_stop_thread(std::size_t /* num_thread */);
        void on_error(
            std::size_t /* num_thread */, std::exception_ptr const& /* e */);

    private:
        thread_queue_init_parameters parameters_;

        // mutex protecting the members
        mutable mutex_type mtx_;

#if defined(PIKA_HAVE_SCHEDULER_THREAD_MAP)
        // mapping of thread id's to pika-threads
        thread_map_type thread_map_;
#endif

        // overall count of work items
        // TODO: Maybe?
        //pika::concurrency::detail::cache_line_data<std::atomic<std::int64_t>>
        //    thread_map_count_;
        std::atomic<std::int64_t> thread_map_count_;

        work_items_type work_items_;    // list of active work items

#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
        // overall wait time of work items
        std::atomic<std::int64_t> work_items_wait_;
        // overall number of work items in queue
        std::atomic<std::int64_t> work_items_wait_count_;
#endif
        // list of terminated threads
        terminated_items_type terminated_items_;
        // count of terminated items
        std::atomic<std::int64_t> terminated_items_count_;

        task_items_type new_tasks_;    // list of new tasks to run

#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
        // overall wait time of new tasks
        std::atomic<std::int64_t> new_tasks_wait_;
        // overall number tasks waited
        std::atomic<std::int64_t> new_tasks_wait_count_;
#endif

        thread_heap_type thread_heap_small_;
        thread_heap_type thread_heap_medium_;
        thread_heap_type thread_heap_large_;
        thread_heap_type thread_heap_huge_;
        thread_heap_type thread_heap_nostack_;

#ifdef PIKA_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::uint64_t add_new_time_;
        std::uint64_t cleanup_terminated_time_;
#endif

#ifdef PIKA_HAVE_THREAD_STEALING_COUNTS
        // # of times our associated worker-thread couldn't find work in work_items
        std::atomic<std::int64_t> pending_misses_;

        // # of times our associated worker-thread looked for work in work_items
        std::atomic<std::int64_t> pending_accesses_;

        // count of work_items stolen from this queue
        std::atomic<std::int64_t> stolen_from_pending_;
        // count of new_tasks stolen from this queue
        std::atomic<std::int64_t> stolen_from_staged_;
        // count of work_items stolen to this queue from other queues
        std::atomic<std::int64_t> stolen_to_pending_;
        // count of new_tasks stolen to this queue from other queues
        std::atomic<std::int64_t> stolen_to_staged_;
#endif
        // count of new tasks to run, separate to new cache line to avoid false
        // sharing
        pika::concurrency::detail::cache_line_data<std::atomic<std::int64_t>>
            new_tasks_count_;

        // count of active work items
        pika::concurrency::detail::cache_line_data<std::atomic<std::int64_t>>
            work_items_count_;
    };
}    // namespace pika::threads::policies
