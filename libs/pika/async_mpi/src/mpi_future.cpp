//  Copyright (c) 2019 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/async_mpi/mpi_exception.hpp>
#include <pika/async_mpi/mpi_future.hpp>
#include <pika/modules/errors.hpp>
#include <pika/modules/threading_base.hpp>
#include <pika/mpi_base/mpi_environment.hpp>
#include <pika/synchronization/mutex.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <mpi.h>

namespace pika { namespace mpi { namespace experimental {

    namespace detail {

        // Holds an MPI_Request and a callback. The callback is intended to be
        // called when the operation tight to the request handle is finished.
        struct request_callback
        {
            MPI_Request request;
            request_callback_function_type callback_function;
        };

        using request_callback_queue_type =
            concurrency::detail::ConcurrentQueue<request_callback>;

        request_callback_queue_type& get_request_callback_queue()
        {
            static request_callback_queue_type request_callback_queue;
            return request_callback_queue;
        }

        // used internally to add an MPI_Request to the lockfree queue
        // that will be used by the polling routines to check when requests
        // have completed
        void add_to_request_callback_queue(request_callback&& req_callback)
        {
            get_request_callback_queue().enqueue(PIKA_MOVE(req_callback));
            ++(get_mpi_info().requests_queue_size_);

            if constexpr (mpi_debug.is_enabled())
            {
                mpi_debug.debug(debug::str<>("request callback queued"),
                    get_mpi_info(), "request",
                    debug::hex<8>(req_callback.request));
            }
        }

#if defined(PIKA_DEBUG)
        std::atomic<std::size_t>& get_register_polling_count()
        {
            static std::atomic<std::size_t> register_polling_count{0};
            return register_polling_count;
        }
#endif

        void add_request_callback(
            request_callback_function_type&& callback, MPI_Request request)
        {
            PIKA_ASSERT_MSG(detail::get_register_polling_count() != 0,
                "MPI event polling has not been enabled on any pool. Make "
                "sure "
                "that MPI event polling is enabled on at least one thread "
                "pool.");

            // Eagerly check if request already completed. If it did, call the
            // callback immediately.
            int flag = 0;
            int result = MPI_Test(&request, &flag, MPI_STATUS_IGNORE);
            if (flag)
            {
                PIKA_INVOKE(PIKA_MOVE(callback), result);
                return;
            }

            detail::add_to_request_callback_queue(
                request_callback{request, PIKA_MOVE(callback)});
        }

        // Mutex is used to serialize polling; only one worker thread does
        // polling at a time. Note that the mpi poll function takes place inside
        // the main scheduling loop of pika and not on an pika worker thread, so
        // we must use std:mutex.
        mutex_type& get_mtx()
        {
            static mutex_type mtx;
            return mtx;
        }

        // an MPI error handling type that we can use to intercept
        // MPI errors if we enable the error handler
        MPI_Errhandler pika_mpi_errhandler = 0;

        // an instance of mpi_info that we store data in
        mpi_info& get_mpi_info()
        {
            static mpi_info mpi_info_;
            return mpi_info_;
        }

        // stream operator to display debug mpi_info
        PIKA_EXPORT std::ostream& operator<<(std::ostream& os, mpi_info const&)
        {
            os << "R " << debug::dec<3>(get_mpi_info().rank_) << "/"
               << debug::dec<3>(get_mpi_info().size_) << " queued requests "
               << debug::dec<3>(get_mpi_info().requests_queue_size_);
            return os;
        }

        // function that converts an MPI error into an exception
        void pika_MPI_Handler(MPI_Comm*, int* errorcode, ...)
        {
            mpi_debug.debug(debug::str<>("pika_MPI_Handler"));
            PIKA_THROW_EXCEPTION(invalid_status, "pika_MPI_Handler",
                detail::error_message(*errorcode));
        }
    }    // namespace detail

    // return a future object from a user supplied MPI_Request
    pika::future<void> get_future(MPI_Request request)
    {
        if (request != MPI_REQUEST_NULL)
        {
            PIKA_ASSERT_MSG(detail::get_register_polling_count() != 0,
                "MPI event polling has not been enabled on any pool. Make "
                "sure "
                "that MPI event polling is enabled on at least one thread "
                "pool.");

            // create a future data shared state with the request Id
            detail::future_data_ptr data(new detail::future_data(
                detail::future_data::init_no_addref{}, request));

            // return a future bound to the shared state
            using traits::future_access;
            return future_access<pika::future<void>>::create(PIKA_MOVE(data));
        }
        return pika::make_ready_future<void>();
    }

    // set an error handler for communicators that will be called
    // on any error instead of the default behavior of program termination
    void set_error_handler()
    {
        mpi_debug.debug(debug::str<>("set_error_handler"));

        MPI_Comm_create_errhandler(
            detail::pika_MPI_Handler, &detail::pika_mpi_errhandler);
        MPI_Comm_set_errhandler(MPI_COMM_WORLD, detail::pika_mpi_errhandler);
    }

    // Background progress function for MPI async operations
    // Checks for completed MPI_Requests and sets mpi::experimental::future
    // ready when found
    pika::threads::policies::detail::polling_status poll()
    {
        using pika::threads::policies::detail::polling_status;

        std::unique_lock<detail::mutex_type> lk(
            detail::get_mtx(), std::try_to_lock);
        if (!lk.owns_lock())
        {
            if constexpr (mpi_debug.is_enabled())
            {
                // for debugging, create a timer
                static auto poll_deb =
                    mpi_debug.make_timer(1, debug::str<>("Poll - lock failed"));
                // output mpi debug info every N seconds
                mpi_debug.timed(poll_deb, detail::get_mpi_info());
            }
            return polling_status::idle;
        }

        if constexpr (mpi_debug.is_enabled())
        {
            // for debugging, create a timer
            static auto poll_deb =
                mpi_debug.make_timer(1, debug::str<>("Poll - lock success"));
            // output mpi debug info every N seconds
            mpi_debug.timed(poll_deb, detail::get_mpi_info());
        }

        {
            // Check requests for completion
            auto& queue = detail::get_request_callback_queue();
            std::size_t max_tries = queue.size_approx();
            detail::request_callback req_callback;
            while (max_tries-- > 0 && queue.try_dequeue(req_callback))
            {
                int flag = 0;
                int result =
                    MPI_Test(&req_callback.request, &flag, MPI_STATUS_IGNORE);
                if (flag)
                {
                    PIKA_INVOKE(
                        PIKA_MOVE(req_callback.callback_function), result);
                    --(detail::get_mpi_info().requests_queue_size_);
                }
                else
                {
                    queue.enqueue(PIKA_MOVE(req_callback));
                }
            }
        }

        return (detail::get_mpi_info().requests_queue_size_ == 0) ?
            polling_status::idle :
            polling_status::busy;
    }

    namespace detail {
        std::size_t get_work_count()
        {
            return detail::get_mpi_info().requests_queue_size_;
        }

        // -------------------------------------------------------------
        void register_polling(pika::threads::thread_pool_base& pool)
        {
#if defined(PIKA_DEBUG)
            ++get_register_polling_count();
#endif
            mpi_debug.debug(debug::str<>("enable polling"));
            auto* sched = pool.get_scheduler();
            sched->set_mpi_polling_functions(
                &pika::mpi::experimental::poll, &get_work_count);
        }

        // -------------------------------------------------------------
        void unregister_polling(pika::threads::thread_pool_base& pool)
        {
#if defined(PIKA_DEBUG)
            {
                bool requests_queue_empty = get_work_count() == 0;
                PIKA_ASSERT_MSG(requests_queue_empty,
                    "MPI request polling was disabled while there are "
                    "unprocessed MPI requests. Make sure MPI request polling "
                    "is not disabled too early.");
            }
#endif
            if constexpr (mpi_debug.is_enabled())
                mpi_debug.debug(debug::str<>("disable polling"));
            auto* sched = pool.get_scheduler();
            sched->clear_mpi_polling_function();
        }
    }    // namespace detail

    // initialize the pika::mpi background request handler
    // All ranks should call this function,
    // but only one thread per rank needs to do so
    void init(
        bool init_mpi, std::string const& pool_name, bool init_errorhandler)
    {
        if (init_mpi)
        {
            int required = MPI_THREAD_MULTIPLE;
            int minimal = MPI_THREAD_FUNNELED;
            int provided;
            pika::util::mpi_environment::init(
                nullptr, nullptr, required, minimal, provided);
            if (provided < MPI_THREAD_FUNNELED)
            {
                mpi_debug.error(debug::str<>("pika::mpi::experimental::init"),
                    "init failed");
                PIKA_THROW_EXCEPTION(invalid_status,
                    "pika::mpi::experimental::init",
                    "the MPI installation doesn't allow multiple threads");
            }
            MPI_Comm_rank(MPI_COMM_WORLD, &detail::get_mpi_info().rank_);
            MPI_Comm_size(MPI_COMM_WORLD, &detail::get_mpi_info().size_);
        }
        else
        {
            // Check if MPI_Init has been called previously
            if (detail::get_mpi_info().size_ == -1)
            {
                int is_initialized = 0;
                MPI_Initialized(&is_initialized);
                if (is_initialized)
                {
                    MPI_Comm_rank(
                        MPI_COMM_WORLD, &detail::get_mpi_info().rank_);
                    MPI_Comm_size(
                        MPI_COMM_WORLD, &detail::get_mpi_info().size_);
                }
            }
        }

        mpi_debug.debug(debug::str<>("pika::mpi::experimental::init"),
            detail::get_mpi_info());

        if (init_errorhandler)
        {
            set_error_handler();
            detail::get_mpi_info().error_handler_initialized_ = true;
        }

        // install polling loop on requested thread pool
        if (pool_name.empty())
        {
            detail::register_polling(pika::resource::get_thread_pool(0));
        }
        else
        {
            detail::register_polling(
                pika::resource::get_thread_pool(pool_name));
        }
    }

    // -----------------------------------------------------------------

    void finalize(std::string const& pool_name)
    {
        if (detail::get_mpi_info().error_handler_initialized_)
        {
            PIKA_ASSERT(detail::pika_mpi_errhandler != 0);
            detail::get_mpi_info().error_handler_initialized_ = false;
            MPI_Errhandler_free(&detail::pika_mpi_errhandler);
            detail::pika_mpi_errhandler = 0;
        }

        // clean up if we initialized mpi
        pika::util::mpi_environment::finalize();

        mpi_debug.debug(debug::str<>("Clearing mode"), detail::get_mpi_info(),
            "disable_user_polling");

        if (pool_name.empty())
        {
            detail::unregister_polling(pika::resource::get_thread_pool(0));
        }
        else
        {
            detail::unregister_polling(
                pika::resource::get_thread_pool(pool_name));
        }
    }
}}}    // namespace pika::mpi::experimental
