//  Copyright (c) 2007-2019 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/functional/function.hpp>
#include <pika/modules/errors.hpp>
#include <pika/modules/format.hpp>
#include <pika/schedulers/deadlock_detection.hpp>
#include <pika/schedulers/maintain_queue_wait_times.hpp>
#include <pika/schedulers/queue_helpers.hpp>
#include <pika/schedulers/thread_queue.hpp>
#include <pika/thread_support/unlock_guard.hpp>
#include <pika/threading_base/scheduler_base.hpp>
#include <pika/threading_base/thread_data.hpp>
#include <pika/threading_base/thread_data_stackful.hpp>
#include <pika/threading_base/thread_data_stackless.hpp>
#include <pika/threading_base/thread_queue_init_parameters.hpp>
#include <pika/timing/high_resolution_clock.hpp>
#include <pika/util/get_and_reset_value.hpp>

#ifdef PIKA_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
#include <pika/timing/tick_counter.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace pika::threads::policies {
    void thread_queue::create_thread_object(
        threads::detail::thread_id_ref_type& thrd,
        threads::detail::thread_init_data& data,
        std::unique_lock<mutex_type>& lk)
    {
        PIKA_ASSERT(lk.owns_lock());

        std::ptrdiff_t const stacksize =
            data.scheduler_base->get_stack_size(data.stacksize);

        thread_heap_type* heap = nullptr;

        if (stacksize == parameters_.small_stacksize_)
        {
            heap = &thread_heap_small_;
        }
        else if (stacksize == parameters_.medium_stacksize_)
        {
            heap = &thread_heap_medium_;
        }
        else if (stacksize == parameters_.large_stacksize_)
        {
            heap = &thread_heap_large_;
        }
        else if (stacksize == parameters_.huge_stacksize_)
        {
            heap = &thread_heap_huge_;
        }
        else if (stacksize == parameters_.nostack_stacksize_)
        {
            heap = &thread_heap_nostack_;
        }
        PIKA_ASSERT(heap);

        if (data.initial_state ==
                threads::detail::thread_schedule_state::
                    pending_do_not_schedule ||
            data.initial_state ==
                threads::detail::thread_schedule_state::pending_boost)
        {
            data.initial_state =
                threads::detail::thread_schedule_state::pending;
        }

        // ASAN gets confused by reusing threads/stacks
#if !defined(PIKA_HAVE_ADDRESS_SANITIZER)

        // Check for an unused thread object.
        if (!heap->empty())
        {
            // Take ownership of the thread object and rebind it.
            thrd = heap->back();
            heap->pop_back();
            get_thread_id_data(thrd)->rebind(data);
        }
        else
#endif
        {
            pika::util::unlock_guard ull(lk);

            // Allocate a new thread object.
            threads::detail::thread_data* p = nullptr;
            if (stacksize == parameters_.nostack_stacksize_)
            {
                p = threads::detail::thread_data_stackless::create(
                    data, this, stacksize);
            }
            else
            {
                p = threads::detail::thread_data_stackful::create(
                    data, this, stacksize);
            }
            thrd = threads::detail::thread_id_ref_type(
                p, threads::detail::thread_id_addref::no);
        }
    }

    std::size_t thread_queue::add_new(std::int64_t add_count,
        thread_queue* addfrom, std::unique_lock<mutex_type>& lk,
        bool /* steal */)
    {
        PIKA_ASSERT(lk.owns_lock());

        if (PIKA_UNLIKELY(0 == add_count))
            return 0;

        std::size_t added = 0;

        while (add_count > 0)
        {
            constexpr std::size_t batch_size = 100;
            std::array<task_description, batch_size> tasks;

            std::size_t added_batch = addfrom->new_tasks_.try_dequeue_bulk(
                std::begin(tasks), batch_size);

            if (added_batch == 0)
            {
                break;
            }

            add_count -= added_batch;

            for (std::size_t i = 0; i < added_batch; ++i)
            {
                task_description& task = tasks[i];

#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
                if (get_maintain_queue_wait_times_enabled())
                {
                    addfrom->new_tasks_wait_ +=
                        pika::chrono::high_resolution_clock::now() -
                        task->waittime;
                    ++addfrom->new_tasks_wait_count_;
                }
#endif
                // create the new thread
                threads::detail::thread_init_data& data = task.data;

                bool schedule_now = data.initial_state ==
                    threads::detail::thread_schedule_state::pending;
                PIKA_UNUSED(schedule_now);

                threads::detail::thread_id_ref_type thrd;
                create_thread_object(thrd, data, lk);

#if defined(PIKA_HAVE_SCHEDULER_THREAD_MAP)
                // add the new entry to the map of all threads
                std::pair<thread_map_type::iterator, bool> p =
                    thread_map_.insert(thrd.noref());

                if (PIKA_UNLIKELY(!p.second))
                {
                    --addfrom->new_tasks_count_.data_;
                    lk.unlock();
                    PIKA_THROW_EXCEPTION(pika::out_of_memory,
                        "thread_queue::add_new",
                        "Couldn't add new thread to the thread map");
                    return 0;
                }
#endif

                ++thread_map_count_;

                // Decrement only after thread_map_count_ has been incremented
                --addfrom->new_tasks_count_.data_;

                // insert the thread into the work-items queue assuming it is
                // in pending state, thread would go out of scope otherwise
                PIKA_ASSERT(schedule_now);

                // pushing the new thread into the pending queue of the
                // specified thread_queue
                ++added;
                schedule_thread(PIKA_MOVE(thrd));
            }
        }

        if (added)
        {
            LTM_(debug).format("add_new: added {} tasks to queues", added);
        }
        return added;
    }

    bool thread_queue::add_new_always(std::size_t& added, thread_queue* addfrom,
        std::unique_lock<mutex_type>& lk, bool steal)
    {
        PIKA_ASSERT(lk.owns_lock());

#ifdef PIKA_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        util::tick_counter tc(add_new_time_);
#endif

        // create new threads from pending tasks (if appropriate)
        std::int64_t add_count = -1;    // default is no constraint

        // if we are desperate (no work in the queues), add some even if the
        // map holds more than max_thread_count
        if (PIKA_LIKELY(parameters_.max_thread_count_))
        {
            std::int64_t count = static_cast<std::int64_t>(thread_map_count_);
            if (parameters_.max_thread_count_ >=
                count + parameters_.min_add_new_count_)
            {    //-V104
                PIKA_ASSERT(parameters_.max_thread_count_ - count <
                    (std::numeric_limits<std::int64_t>::max)());
                add_count = static_cast<std::int64_t>(
                    parameters_.max_thread_count_ - count);
                if (add_count < parameters_.min_add_new_count_)
                    add_count = parameters_.min_add_new_count_;
                if (add_count > parameters_.max_add_new_count_)
                    add_count = parameters_.max_add_new_count_;
            }
            else if (work_items_.size_approx() == 0)
            {
                // add this number of threads
                add_count = parameters_.min_add_new_count_;

                // increase max_thread_count
                parameters_.max_thread_count_ +=
                    parameters_.min_add_new_count_;    //-V101
            }
            else
            {
                return false;
            }
        }

        std::size_t addednew = add_new(add_count, addfrom, lk, steal);
        added += addednew;
        return addednew != 0;
    }

    void thread_queue::recycle_thread(threads::detail::thread_id_type thrd)
    {
        std::ptrdiff_t stacksize = get_thread_id_data(thrd)->get_stack_size();

        if (stacksize == parameters_.small_stacksize_)
        {
            thread_heap_small_.push_back(thrd);
        }
        else if (stacksize == parameters_.medium_stacksize_)
        {
            thread_heap_medium_.push_back(thrd);
        }
        else if (stacksize == parameters_.large_stacksize_)
        {
            thread_heap_large_.push_back(thrd);
        }
        else if (stacksize == parameters_.huge_stacksize_)
        {
            thread_heap_huge_.push_back(thrd);
        }
        else if (stacksize == parameters_.nostack_stacksize_)
        {
            thread_heap_nostack_.push_back(thrd);
        }
        else
        {
            PIKA_ASSERT_MSG(
                false, util::format("Invalid stack size {1}", stacksize));
        }
    }

    /// This function makes sure all threads which are marked for deletion
    /// (state is terminated) are properly destroyed.
    ///
    /// This returns 'true' if there are no more terminated threads waiting
    /// to be deleted.
    bool thread_queue::cleanup_terminated_locked(bool delete_all)
    {
#ifdef PIKA_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        util::tick_counter tc(cleanup_terminated_time_);
#endif

        if (terminated_items_count_.load(std::memory_order_acquire) == 0)
            return true;

        constexpr std::size_t batch_size = 10;
        std::array<threads::detail::thread_data*, batch_size> todeletes;

        if (delete_all)
        {
            // delete all threads
            while (std::size_t dequeued = terminated_items_.try_dequeue_bulk(
                       std::begin(todeletes), batch_size))
            {
                for (std::size_t i = 0; i < dequeued; ++i)
                {
                    threads::detail::thread_data* todelete = todeletes[i];
                    threads::detail::thread_id_type tid(todelete);
                    --terminated_items_count_;

#if defined(PIKA_HAVE_SCHEDULER_THREAD_MAP)
                    // this thread has to be in this map
                    PIKA_ASSERT(thread_map_.find(tid) != thread_map_.end());
                    auto erased = thread_map_.erase(tid);
                    PIKA_ASSERT(erased);
                    PIKA_UNUSED(erased);
#endif

                    recycle_thread(tid);
                    --thread_map_count_;
                    PIKA_ASSERT(thread_map_count_ >= 0);
                }
            }
        }
        else
        {
            // delete only this many threads
            std::int64_t delete_count = (std::min)(
                static_cast<std::int64_t>(terminated_items_count_ / 10),
                static_cast<std::int64_t>(parameters_.max_delete_count_));

            // delete at least this many threads
            delete_count = (std::max)(delete_count,
                static_cast<std::int64_t>(parameters_.min_delete_count_));

            while (delete_count)
            {
                std::size_t dequeued =
                    terminated_items_.try_dequeue_bulk(std::begin(todeletes),
                        (std::min)(delete_count,
                            static_cast<std::int64_t>(batch_size)));
                if (dequeued == 0)
                {
                    break;
                }

                for (std::size_t i = 0; i < dequeued; ++i)
                {
                    threads::detail::thread_data* todelete = todeletes[i];
                    threads::detail::thread_id_type tid(todelete);
                    --terminated_items_count_;

                    // this thread has to be in this map, except if it has changed
                    // its priority, then it could be elsewhere

#if defined(PIKA_HAVE_SCHEDULER_THREAD_MAP)
                    // this thread has to be in this map
                    PIKA_ASSERT(thread_map_.find(tid) != thread_map_.end());
                    auto erased = thread_map_.erase(tid);
                    PIKA_ASSERT(erased);
                    PIKA_UNUSED(erased);
#endif

                    recycle_thread(tid);
                    --thread_map_count_;
                    PIKA_ASSERT(thread_map_count_ >= 0);
                    --delete_count;
                }
            }
        }
        return terminated_items_count_.load(std::memory_order_acquire) == 0;
    }

    bool thread_queue::cleanup_terminated(bool delete_all)
    {
        if (terminated_items_count_.load(std::memory_order_acquire) == 0)
            return true;

        if (delete_all)
        {
            // do not lock mutex while deleting all threads, do it piece-wise
            while (true)
            {
                std::lock_guard<mutex_type> lk(mtx_);
                if (cleanup_terminated_locked(false))
                {
                    return true;
                }
            }
            return false;
        }

        std::lock_guard<mutex_type> lk(mtx_);
        return cleanup_terminated_locked(false);
    }

    thread_queue::thread_queue(
        std::size_t /* queue_num */, thread_queue_init_parameters parameters)
      : parameters_(parameters)
      , thread_map_count_(0)
      , work_items_(128)
#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
      , work_items_wait_(0)
      , work_items_wait_count_(0)
#endif
      , terminated_items_(128)
      , terminated_items_count_(0)
      , new_tasks_(128)
#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
      , new_tasks_wait_(0)
      , new_tasks_wait_count_(0)
#endif
      , thread_heap_small_()
      , thread_heap_medium_()
      , thread_heap_large_()
      , thread_heap_huge_()
      , thread_heap_nostack_()
#ifdef PIKA_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
      , add_new_time_(0)
      , cleanup_terminated_time_(0)
#endif
#ifdef PIKA_HAVE_THREAD_STEALING_COUNTS
      , pending_misses_(0)
      , pending_accesses_(0)
      , stolen_from_pending_(0)
      , stolen_from_staged_(0)
      , stolen_to_pending_(0)
      , stolen_to_staged_(0)
#endif
    {
        new_tasks_count_.data_ = 0;
        work_items_count_.data_ = 0;

        thread_heap_small_.reserve(128);
        thread_heap_medium_.reserve(128);
        thread_heap_large_.reserve(128);
        thread_heap_huge_.reserve(128);
        thread_heap_nostack_.reserve(128);
    }

    void thread_queue::deallocate(threads::detail::thread_data* p)
    {
        p->destroy();
    }

    thread_queue::~thread_queue()
    {
        for (auto t : thread_heap_small_)
            deallocate(get_thread_id_data(t));

        for (auto t : thread_heap_medium_)
            deallocate(get_thread_id_data(t));

        for (auto t : thread_heap_large_)
            deallocate(get_thread_id_data(t));

        for (auto t : thread_heap_huge_)
            deallocate(get_thread_id_data(t));

        for (auto t : thread_heap_nostack_)
            deallocate(get_thread_id_data(t));
    }

#ifdef PIKA_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
    std::uint64_t thread_queue::get_creation_time(bool reset)
    {
        return util::get_and_reset_value(add_new_time_, reset);
    }

    std::uint64_t thread_queue::get_cleanup_time(bool reset)
    {
        return util::get_and_reset_value(cleanup_terminated_time_, reset);
    }
#endif

    std::int64_t thread_queue::get_queue_length(std::memory_order order) const
    {
        return work_items_count_.data_.load(order) +
            new_tasks_count_.data_.load(order);
    }

    std::int64_t thread_queue::get_pending_queue_length(
        std::memory_order order) const
    {
        return work_items_count_.data_.load(order);
    }

    std::int64_t thread_queue::get_staged_queue_length(
        std::memory_order order) const
    {
        return new_tasks_count_.data_.load(order);
    }

#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
    std::uint64_t thread_queue::get_average_task_wait_time() const
    {
        std::uint64_t count = new_tasks_wait_count_;
        if (count == 0)
            return 0;
        return new_tasks_wait_ / count;
    }

    std::uint64_t thread_queue::get_average_thread_wait_time() const
    {
        std::uint64_t count = work_items_wait_count_;
        if (count == 0)
            return 0;
        return work_items_wait_ / count;
    }
#endif

#ifdef PIKA_HAVE_THREAD_STEALING_COUNTS
    std::int64_t thread_queue::get_num_pending_misses(bool reset)
    {
        return util::get_and_reset_value(pending_misses_, reset);
    }

    void thread_queue::increment_num_pending_misses(std::size_t num)
    {
        pending_misses_.fetch_add(num, std::memory_order_relaxed);
    }

    std::int64_t thread_queue::get_num_pending_accesses(bool reset)
    {
        return util::get_and_reset_value(pending_accesses_, reset);
    }

    void thread_queue::increment_num_pending_accesses(std::size_t num)
    {
        pending_accesses_.fetch_add(num, std::memory_order_relaxed);
    }

    std::int64_t thread_queue::get_num_stolen_from_pending(bool reset)
    {
        return util::get_and_reset_value(stolen_from_pending_, reset);
    }

    void thread_queue::increment_num_stolen_from_pending(std::size_t num)
    {
        stolen_from_pending_.fetch_add(num, std::memory_order_relaxed);
    }

    std::int64_t thread_queue::get_num_stolen_from_staged(bool reset)
    {
        return util::get_and_reset_value(stolen_from_staged_, reset);
    }

    void thread_queue::increment_num_stolen_from_staged(std::size_t num)
    {
        stolen_from_staged_.fetch_add(num, std::memory_order_relaxed);
    }

    std::int64_t thread_queue::get_num_stolen_to_pending(bool reset)
    {
        return util::get_and_reset_value(stolen_to_pending_, reset);
    }

    void thread_queue::increment_num_stolen_to_pending(std::size_t num)
    {
        stolen_to_pending_.fetch_add(num, std::memory_order_relaxed);
    }

    std::int64_t thread_queue::get_num_stolen_to_staged(bool reset)
    {
        return util::get_and_reset_value(stolen_to_staged_, reset);
    }

    void thread_queue::increment_num_stolen_to_staged(std::size_t num)
    {
        stolen_to_staged_.fetch_add(num, std::memory_order_relaxed);
    }
#else
    void thread_queue::increment_num_pending_misses(std::size_t /* num */) {}
    void thread_queue::increment_num_pending_accesses(std::size_t /* num */) {}
    void thread_queue::increment_num_stolen_from_pending(std::size_t /* num */)
    {
    }
    void thread_queue::increment_num_stolen_from_staged(std::size_t /* num */)
    {
    }
    void thread_queue::increment_num_stolen_to_pending(std::size_t /* num */) {}
    void thread_queue::increment_num_stolen_to_staged(std::size_t /* num */) {}
#endif

    void thread_queue::create_thread(threads::detail::thread_init_data& data,
        threads::detail::thread_id_ref_type* id, error_code& ec)
    {
        // thread has not been created yet
        if (id)
            *id = threads::detail::invalid_thread_id;

        if (data.stacksize == execution::thread_stacksize::current)
        {
            data.stacksize = threads::detail::get_self_stacksize_enum();
        }

        PIKA_ASSERT(data.stacksize != execution::thread_stacksize::current);

        if (data.run_now)
        {
            threads::detail::thread_id_ref_type thrd;

            // The mutex can not be locked while a new thread is getting
            // created, as it might have that the current pika thread gets
            // suspended.
            {
                std::unique_lock<mutex_type> lk(mtx_);

                bool schedule_now = data.initial_state ==
                    threads::detail::thread_schedule_state::pending;

                create_thread_object(thrd, data, lk);

#if defined(PIKA_HAVE_SCHEDULER_THREAD_MAP)
                // add a new entry in the map for this thread
                std::pair<thread_map_type::iterator, bool> p =
                    thread_map_.insert(thrd.noref());

                if (PIKA_UNLIKELY(!p.second))
                {
                    lk.unlock();
                    PIKA_THROWS_IF(ec, pika::out_of_memory,
                        "thread_queue::create_thread",
                        "Couldn't add new thread to the map of "
                        "threads");
                    return;
                }

                // this thread has to be in the map now
                PIKA_ASSERT(
                    thread_map_.find(thrd.noref()) != thread_map_.end());
#endif
                ++thread_map_count_;

                PIKA_ASSERT(
                    &get_thread_id_data(thrd)->get_queue<thread_queue>() ==
                    this);

                // push the new thread in the pending thread queue
                if (schedule_now)
                {
                    // return the thread_id_ref of the newly created thread
                    if (id)
                    {
                        *id = thrd;
                    }
                    schedule_thread(PIKA_MOVE(thrd));
                }
                else
                {
                    // if the thread should not be scheduled the id must be
                    // returned to the caller as otherwise the thread would
                    // go out of scope right away.
                    PIKA_ASSERT(id != nullptr);
                    *id = PIKA_MOVE(thrd);
                }

                if (&ec != &throws)
                    ec = make_success_code();
                return;
            }
        }

        // if the initial state is not pending, delayed creation will
        // fail as the newly created thread would go out of scope right
        // away (can't be scheduled).
        if (data.initial_state !=
            threads::detail::thread_schedule_state::pending)
        {
            PIKA_THROW_EXCEPTION(bad_parameter, "thread_queue::create_thread",
                "staged tasks must have 'pending' as their initial "
                "state");
        }

        // do not execute the work, but register a task description for
        // later thread creation
        ++new_tasks_count_.data_;

        task_description td{
            PIKA_MOVE(data),
#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
            pika::chrono::high_resolution_clock::now(),
#endif
        };
        new_tasks_.enqueue(PIKA_MOVE(td));
        if (&ec != &throws)
            ec = make_success_code();
    }

    bool thread_queue::get_next_thread(
        threads::detail::thread_id_ref_type& thrd, bool allow_stealing,
        bool /* steal */)
    {
        std::int64_t work_items_count =
            work_items_count_.data_.load(std::memory_order_relaxed);

        if (allow_stealing &&
            parameters_.min_tasks_to_steal_pending_ > work_items_count)
        {
            return false;
        }

#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
        thread_description_ptr tdesc;
        if (0 != work_items_count && work_items_.try_dequeue(tdesc))
        {
            --work_items_count_.data_;

            if (get_maintain_queue_wait_times_enabled())
            {
                work_items_wait_ += pika::chrono::high_resolution_clock::now() -
                    tdesc->waittime;
                ++work_items_wait_count_;
            }

            thrd = PIKA_MOVE(tdesc->data);
            delete tdesc;

            return true;
        }
#else
        thread_description_ptr next_thrd;
        if (0 != work_items_count && work_items_.try_dequeue(next_thrd))
        {
            thrd.reset(next_thrd, false);    // do not addref!
            --work_items_count_.data_;
            return true;
        }
#endif
        return false;
    }

    void thread_queue::schedule_thread(
        threads::detail::thread_id_ref_type thrd, bool /* other_end */)
    {
        ++work_items_count_.data_;
#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
        work_items_.enqueue(thread_description{
            PIKA_MOVE(thrd), pika::chrono::high_resolution_clock::now()});
#else
        // detach the thread from the id_ref without decrementing
        // the reference count
        work_items_.enqueue(thrd.detach());
#endif
    }

    void thread_queue::destroy_thread(threads::detail::thread_data* thrd)
    {
        PIKA_ASSERT(&thrd->get_queue<thread_queue>() == this);

        terminated_items_.enqueue(thrd);

        std::int64_t count = ++terminated_items_count_;
        if (count > parameters_.max_terminated_threads_)
        {
            cleanup_terminated(true);    // clean up all terminated threads
        }
    }

    std::int64_t thread_queue::get_thread_count(
        threads::detail::thread_schedule_state state) const
    {
        if (threads::detail::thread_schedule_state::terminated == state)
            return terminated_items_count_;

        if (threads::detail::thread_schedule_state::staged == state)
            return new_tasks_count_.data_;

        if (threads::detail::thread_schedule_state::unknown == state)
        {
            return thread_map_count_ + new_tasks_count_.data_ -
                terminated_items_count_;
        }

#if defined(PIKA_HAVE_SCHEDULER_THREAD_MAP)
        // acquire lock only if absolutely necessary
        std::lock_guard<mutex_type> lk(mtx_);

        std::int64_t num_threads = 0;
        thread_map_type::const_iterator end = thread_map_.end();
        for (thread_map_type::const_iterator it = thread_map_.begin();
             it != end; ++it)
        {
            if (get_thread_id_data(*it)->get_state().state() == state)
                ++num_threads;
        }

        return num_threads;
#else
        // All created threads are in thread_map_count_ but suspended and
        // currently running threads are missing from work_items_count_. We
        // estimate suspended threads by the difference so the count may be
        // off by one.
        if (threads::detail::thread_schedule_state::suspended == state)
        {
            return thread_map_count_ - work_items_count_.data_;
        }

        PIKA_THROW_EXCEPTION(bad_parameter, "thread_queue::get_thread_count",
            "PIKA_HAVE_SCHEDULER_THREAD_MAP is disabled, can't get thread "
            "count for given state {}",
            state);    // TODO: Format given state into error string

        return std::size_t(-1);
#endif
    }

    void thread_queue::abort_all_suspended_threads()
    {
#if defined(PIKA_HAVE_SCHEDULER_THREAD_MAP)
        std::lock_guard<mutex_type> lk(mtx_);
        thread_map_type::iterator end = thread_map_.end();
        for (thread_map_type::iterator it = thread_map_.begin(); it != end;
             ++it)
        {
            auto thrd = get_thread_id_data(*it);
            if (thrd->get_state().state() ==
                threads::detail::thread_schedule_state::suspended)
            {
                thrd->set_state(threads::detail::thread_schedule_state::pending,
                    threads::detail::thread_restart_state::abort);

                // thread holds self-reference
                PIKA_ASSERT(thrd->count_ > 1);
                schedule_thread(threads::detail::thread_id_ref_type(thrd));
            }
        }
#else
        PIKA_THROW_EXCEPTION(bad_parameter,
            "thread_queue::abort_all_suspended_threads",
            "PIKA_HAVE_SCHEDULER_THREAD_MAP is disabled, can't abort "
            "suspended threads");
#endif
    }

    bool thread_queue::enumerate_threads(
        util::function<bool(threads::detail::thread_id_type)> const& f,
        threads::detail::thread_schedule_state state) const
    {
#if defined(PIKA_HAVE_SCHEDULER_THREAD_MAP)
        std::uint64_t count = thread_map_count_;
        if (state == threads::detail::thread_schedule_state::terminated)
        {
            count = terminated_items_count_;
        }
        else if (state == threads::detail::thread_schedule_state::staged)
        {
            PIKA_THROW_EXCEPTION(bad_parameter, "thread_queue::iterate_threads",
                "can't iterate over thread ids of staged threads");
            return false;
        }

        std::vector<threads::detail::thread_id_type> ids;
        ids.reserve(static_cast<std::size_t>(count));

        if (state == threads::detail::thread_schedule_state::unknown)
        {
            std::lock_guard<mutex_type> lk(mtx_);
            thread_map_type::const_iterator end = thread_map_.end();
            for (thread_map_type::const_iterator it = thread_map_.begin();
                 it != end; ++it)
            {
                ids.push_back(*it);
            }
        }
        else
        {
            std::lock_guard<mutex_type> lk(mtx_);
            thread_map_type::const_iterator end = thread_map_.end();
            for (thread_map_type::const_iterator it = thread_map_.begin();
                 it != end; ++it)
            {
                if (get_thread_id_data(*it)->get_state().state() == state)
                    ids.push_back(*it);
            }
        }

        // now invoke callback function for all matching threads
        for (threads::detail::thread_id_type const& id : ids)
        {
            if (!f(id))
                return false;    // stop iteration
        }
#else
        PIKA_UNUSED(f);
        PIKA_UNUSED(state);
        // TODO: bad_parameter?
        PIKA_THROW_EXCEPTION(bad_parameter, "thread_queue::enumerate_threads",
            "PIKA_HAVE_SCHEDULER_THREAD_MAP is disabled, can't enumerate "
            "threads");
#endif

        return true;
    }

    bool thread_queue::wait_or_add_new(bool, std::size_t& added, bool steal)
    {
        if (0 == new_tasks_count_.data_.load(std::memory_order_relaxed))
        {
            return true;
        }

        // No obvious work has to be done, so a lock won't hurt too much.
        //
        // We prefer to exit this function (some kind of very short
        // busy waiting) to blocking on this lock. Locking fails either
        // when a thread is currently doing thread maintenance, which
        // means there might be new work, or the thread owning the lock
        // just falls through to the cleanup work below (no work is available)
        // in which case the current thread (which failed to acquire
        // the lock) will just retry to enter this loop.
        std::unique_lock<mutex_type> lk(mtx_, std::try_to_lock);
        if (!lk.owns_lock())
            return false;    // avoid long wait on lock

        // stop running after all pika threads have been terminated
        return !add_new_always(added, this, lk, steal);
    }

    bool thread_queue::wait_or_add_new(
        bool running, std::size_t& added, thread_queue* addfrom, bool steal)
    {
        // try to generate new threads from task lists, but only if our
        // own list of threads is empty
        if (0 == work_items_count_.data_.load(std::memory_order_relaxed))
        {
            // see if we can avoid grabbing the lock below

            // don't try to steal if there are only a few tasks left on
            // this queue
            std::int64_t new_tasks_count =
                addfrom->new_tasks_count_.data_.load(std::memory_order_relaxed);
            bool enough_threads =
                new_tasks_count >= parameters_.min_tasks_to_steal_staged_;

            if (running && !enough_threads)
            {
                if (new_tasks_count != 0)
                {
                    LTM_(debug).format("thread_queue::wait_or_add_"
                                       "new: not enough threads "
                                       "to steal from queue {} to "
                                       "queue {}, have {} but "
                                       "need at least {}",
                        addfrom, this, new_tasks_count,
                        parameters_.min_tasks_to_steal_staged_);
                }

                return false;
            }

            // No obvious work has to be done, so a lock won't hurt too much.
            //
            // We prefer to exit this function (some kind of very short
            // busy waiting) to blocking on this lock. Locking fails either
            // when a thread is currently doing thread maintenance, which
            // means there might be new work, or the thread owning the lock
            // just falls through to the cleanup work below (no work is available)
            // in which case the current thread (which failed to acquire
            // the lock) will just retry to enter this loop.
            std::unique_lock<mutex_type> lk(mtx_, std::try_to_lock);
            if (!lk.owns_lock())
                return false;    // avoid long wait on lock

            // stop running after all pika threads have been terminated
            bool added_new = add_new_always(added, addfrom, lk, steal);
            if (!added_new)
            {
                // Before exiting each of the OS threads deletes the
                // remaining terminated pika threads
                // REVIEW: Should we be doing this if we are stealing?
                bool canexit = cleanup_terminated_locked(true);
                if (!running && canexit)
                {
                    // we don't have any registered work items anymore
                    //do_some_work();       // notify possibly waiting threads
                    return true;    // terminate scheduling loop
                }
                return false;
            }
            else
            {
                cleanup_terminated_locked();
                return false;
            }
        }

        bool canexit = cleanup_terminated(true);
        if (!running && canexit)
        {
            // we don't have any registered work items anymore
            return true;    // terminate scheduling loop
        }

        return false;
    }

    bool thread_queue::dump_suspended_threads(
        std::size_t num_thread, std::int64_t& idle_loop_count, bool running)
    {
#if !defined(PIKA_HAVE_SCHEDULER_THREAD_MAP) ||                                \
    !defined(PIKA_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION)
        PIKA_UNUSED(num_thread);
        PIKA_UNUSED(idle_loop_count);
        PIKA_UNUSED(running);
        return false;
#else
        if (get_minimal_deadlock_detection_enabled())
        {
            std::lock_guard<mutex_type> lk(mtx_);
            return detail::dump_suspended_threads(
                num_thread, thread_map_, idle_loop_count, running);
        }
        return false;
#endif
    }

    void thread_queue::on_start_thread(std::size_t /* num_thread */)
    {
        thread_heap_small_.reserve(parameters_.init_threads_count_);
        thread_heap_medium_.reserve(parameters_.init_threads_count_);
        thread_heap_large_.reserve(parameters_.init_threads_count_);
        thread_heap_huge_.reserve(parameters_.init_threads_count_);

        // Pre-allocate init_threads_count threads, with accompanying stack,
        // with the default stack size
        static_assert(execution::thread_stacksize::default_ ==
                execution::thread_stacksize::small_,
            "This assumes that the default stacksize is "
            "\"small_\". If the "
            "default changes, so should this code. If this "
            "static_assert "
            "fails you've most likely changed the default without "
            "changing "
            "the code here.");

        std::lock_guard<mutex_type> lk(mtx_);
        for (std::int64_t i = 0; i < parameters_.init_threads_count_; ++i)
        {
            // We don't care about the init parameters since this thread
            // will be rebound once it is actually used
            threads::detail::thread_init_data init_data;

            // We start the reference count at zero since the thread goes
            // immediately into the list of recycled threads
            threads::detail::thread_data* p =
                threads::detail::thread_data_stackful::create(init_data, this,
                    parameters_.small_stacksize_,
                    threads::detail::thread_id_addref::no);
            PIKA_ASSERT(p);

            // We initialize the stack eagerly
            p->init();

            // Finally, store the thread for later use
            thread_heap_small_.emplace_back(p);
        }
    }
    void thread_queue::on_stop_thread(std::size_t /* num_thread */) {}
    void thread_queue::on_error(
        std::size_t /* num_thread */, std::exception_ptr const& /* e */)
    {
    }
}    // namespace pika::threads::policies
