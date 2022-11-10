//  Copyright (c) 2015-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/futures/detail/future_data.hpp>
#include <pika/futures/future.hpp>

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/async_base/launch_policy.hpp>
#include <pika/errors/try_catch_exception_ptr.hpp>
#include <pika/execution_base/this_thread.hpp>
#include <pika/functional/deferred_call.hpp>
#include <pika/functional/unique_function.hpp>
#include <pika/futures/futures_factory.hpp>
#include <pika/modules/errors.hpp>
#include <pika/modules/memory.hpp>
#include <pika/threading_base/annotated_function.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <utility>

namespace pika { namespace lcos { namespace detail {

    static run_on_completed_error_handler_type run_on_completed_error_handler;

    void set_run_on_completed_error_handler(
        run_on_completed_error_handler_type f)
    {
        run_on_completed_error_handler = f;
    }

    future_data_refcnt_base::~future_data_refcnt_base() = default;

    ///////////////////////////////////////////////////////////////////////////
    struct handle_continuation_recursion_count
    {
        handle_continuation_recursion_count() noexcept
          : count_(threads::detail::get_continuation_recursion_count())
        {
            ++count_;
        }
        ~handle_continuation_recursion_count()
        {
            --count_;
        }

        std::size_t& count_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Callback>
    static void run_on_completed_on_new_thread(Callback&& f)
    {
        lcos::local::futures_factory<void()> p(PIKA_FORWARD(Callback, f));

        bool is_pika_thread = nullptr != pika::threads::detail::get_self_ptr();
        pika::launch policy = launch::fork;
        if (!is_pika_thread)
        {
            policy = launch::async;
        }

        policy.set_priority(execution::thread_priority::boost);
        policy.set_stacksize(execution::thread_stacksize::current);

        // launch a new thread executing the given function
        threads::detail::thread_id_ref_type tid =
            p.apply("run_on_completed_on_new_thread", policy);

        // wait for the task to run
        if (is_pika_thread)
        {
            // make sure this thread is executed last
            this_thread::suspend(
                threads::detail::thread_schedule_state::pending, tid.noref());
            return p.get_future().get();
        }

        // If we are not on a pika thread, we need to return immediately, to
        // allow the newly spawned thread to execute.
    }

    ///////////////////////////////////////////////////////////////////////////
    future_data_base<traits::detail::future_data_void>::~future_data_base() =
        default;

    static util::detail::unused_type unused_;

    util::detail::unused_type*
    future_data_base<traits::detail::future_data_void>::get_result_void(
        void const* storage, error_code& ec)
    {
        // yields control if needed
        state s = wait(ec);
        if (ec)
        {
            return nullptr;
        }

        // No locking is required. Once a future has been made ready, which
        // is a postcondition of wait, either:
        //
        // - there is only one writer (future), or
        // - there are multiple readers only (shared_future, lock hurts
        //   concurrency)

        // Avoid retrieving state twice. If wait() returns 'empty' then this
        // thread was suspended, in this case we need to load it again.
        if (s == empty)
        {
            s = state_.load(std::memory_order_relaxed);
        }

        if (s == value)
        {
            return &unused_;
        }

        if (s == empty)
        {
            // the value has already been moved out of this future
            PIKA_THROWS_IF(ec, pika::error::future_already_retrieved,
                "future_data_base::get_result",
                "this future has no valid shared state");
            return nullptr;
        }

        // the thread has been re-activated by one of the actions
        // supported by this promise (see promise::set_event
        // and promise::set_exception).
        if (s == exception)
        {
            std::exception_ptr const* exception_ptr =
                static_cast<std::exception_ptr const*>(storage);

            // an error has been reported in the meantime, throw or set
            // the error code
            if (&ec == &throws)
            {
                std::rethrow_exception(*exception_ptr);
                // never reached
            }
            else
            {
                ec = make_error_code(*exception_ptr);
            }
        }

        return nullptr;
    }

    // deferred execution of a given continuation
    void future_data_base<traits::detail::future_data_void>::run_on_completed(
        completed_callback_type&& on_completed) noexcept
    {
        pika::detail::try_catch_exception_ptr(
            [&]() {
                pika::scoped_annotation annotate(on_completed);
                on_completed();
            },
            [&](std::exception_ptr ep) {
                // If the completion handler throws an exception, there's nothing
                // we can do, report the exception and terminate.
                if (run_on_completed_error_handler)
                {
                    run_on_completed_error_handler(PIKA_MOVE(ep));
                }
                else
                {
                    std::terminate();
                }
            });
    }

    void future_data_base<traits::detail::future_data_void>::run_on_completed(
        completed_callback_vector_type&& on_completed) noexcept
    {
        for (auto&& func : on_completed)
        {
            run_on_completed(PIKA_MOVE(func));
        }
    }

    // make sure continuation invocation does not recurse deeper than
    // allowed
    template <typename Callback>
    void
    future_data_base<traits::detail::future_data_void>::handle_on_completed(
        Callback&& on_completed)
    {
        // We need to run the completion on a new thread if we are on a
        // non pika thread.
#if defined(PIKA_HAVE_THREADS_GET_STACK_POINTER)
        bool recurse_asynchronously =
            !this_thread::has_sufficient_stack_space();
#else
        handle_continuation_recursion_count cnt;
        bool recurse_asynchronously =
            cnt.count_ > PIKA_CONTINUATION_MAX_RECURSION_DEPTH ||
            (pika::threads::detail::get_self_ptr() == nullptr);
#endif
        if (!recurse_asynchronously)
        {
            // directly execute continuation on this thread
            run_on_completed(PIKA_FORWARD(Callback, on_completed));
        }
        else
        {
            // re-spawn continuation on a new thread

            pika::detail::try_catch_exception_ptr(
                [&]() {
                    constexpr void (*p)(Callback &&) =
                        &future_data_base::run_on_completed;
                    run_on_completed_on_new_thread(util::detail::deferred_call(
                        p, PIKA_FORWARD(Callback, on_completed)));
                },
                [&](std::exception_ptr ep) {
                    // If an exception while creating the new task or inside the
                    // completion handler is thrown, there is nothing we can do...
                    // ... but terminate and report the error
                    if (run_on_completed_error_handler)
                    {
                        run_on_completed_error_handler(PIKA_MOVE(ep));
                    }
                    else
                    {
                        std::rethrow_exception(PIKA_MOVE(ep));
                    }
                });
        }
    }

    // We need only one explicit instantiation here as the second version
    // (single callback) is implicitly instantiated below.
    using completed_callback_vector_type =
        future_data_refcnt_base::completed_callback_vector_type;

    template PIKA_EXPORT void
    future_data_base<traits::detail::future_data_void>::handle_on_completed<
        completed_callback_vector_type>(completed_callback_vector_type&&);

    /// Set the callback which needs to be invoked when the future becomes
    /// ready. If the future is ready the function will be invoked
    /// immediately.
    void future_data_base<traits::detail::future_data_void>::set_on_completed(
        completed_callback_type data_sink)
    {
        if (!data_sink)
            return;

        if (is_ready())
        {
            // invoke the callback (continuation) function right away
            handle_on_completed(PIKA_MOVE(data_sink));
        }
        else
        {
            std::unique_lock l(mtx_);
            if (is_ready())
            {
                l.unlock();

                // invoke the callback (continuation) function
                handle_on_completed(PIKA_MOVE(data_sink));
            }
            else
            {
                on_completed_.push_back(PIKA_MOVE(data_sink));
            }
        }
    }

    future_data_base<traits::detail::future_data_void>::state
    future_data_base<traits::detail::future_data_void>::wait(error_code& ec)
    {
        // block if this entry is empty
        state s = state_.load(std::memory_order_acquire);
        if (s == empty)
        {
            std::unique_lock l(mtx_);
            s = state_.load(std::memory_order_relaxed);
            if (s == empty)
            {
                cond_.wait(l, "future_data_base::wait", ec);
                if (ec)
                {
                    return s;
                }

                // reload the state, it's not empty anymore
                s = state_.load(std::memory_order_relaxed);
            }
        }

        if (&ec != &throws)
        {
            ec = make_success_code();
        }
        return s;
    }

    pika::future_status
    future_data_base<traits::detail::future_data_void>::wait_until(
        std::chrono::steady_clock::time_point const& abs_time, error_code& ec)
    {
        // block if this entry is empty
        if (state_.load(std::memory_order_acquire) == empty)
        {
            std::unique_lock l(mtx_);
            if (state_.load(std::memory_order_relaxed) == empty)
            {
                threads::detail::thread_restart_state const reason =
                    cond_.wait_until(
                        l, abs_time, "future_data_base::wait_until", ec);
                if (ec)
                {
                    return pika::future_status::uninitialized;
                }

                if (reason == threads::detail::thread_restart_state::timeout &&
                    state_.load(std::memory_order_acquire) == empty)
                {
                    return pika::future_status::timeout;
                }
            }
        }

        if (&ec != &throws)
        {
            ec = make_success_code();
        }
        return pika::future_status::ready;    //-V110
    }
}}}    // namespace pika::lcos::detail
