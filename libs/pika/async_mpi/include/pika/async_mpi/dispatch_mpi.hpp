//  Copyright (c) 2023 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/transform_xxx.hpp

#pragma once

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/async_mpi/mpi_helpers.hpp>
#include <pika/async_mpi/mpi_polling.hpp>
#include <pika/concepts/concepts.hpp>
#include <pika/datastructures/variant.hpp>
#include <pika/debugging/demangle_helper.hpp>
#include <pika/debugging/print.hpp>
#include <pika/execution/algorithms/detail/helpers.hpp>
#include <pika/execution/algorithms/detail/partial_algorithm.hpp>
#include <pika/execution/algorithms/transfer.hpp>
#include <pika/execution_base/any_sender.hpp>
#include <pika/execution_base/receiver.hpp>
#include <pika/execution_base/sender.hpp>
#include <pika/executors/thread_pool_scheduler.hpp>
#include <pika/functional/detail/tag_fallback_invoke.hpp>
#include <pika/functional/invoke.hpp>
#include <pika/mpi_base/mpi.hpp>

#include <mpi-ext.h>

#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pika::mpi::experimental::detail {
    namespace ex = execution::experimental;

    // -----------------------------------------------------------------
    // route calls through an impl layer for ADL isolation
    template <typename Sender, typename F>
    struct dispatch_mpi_sender_impl
    {
        struct dispatch_mpi_sender_type;
    };

    template <typename Sender, typename F>
    using dispatch_mpi_sender =
        typename dispatch_mpi_sender_impl<Sender, F>::dispatch_mpi_sender_type;

    // -----------------------------------------------------------------
    // transform MPI adapter - sender type
    template <typename Sender, typename F>
    struct dispatch_mpi_sender_impl<Sender, F>::dispatch_mpi_sender_type
    {
        using is_sender = void;

        std::decay_t<Sender> sender;
        std::decay_t<F> f;
        stream_type stream;

#if defined(PIKA_HAVE_STDEXEC)

        using completion_signatures = pika::execution::experimental::completion_signatures<
            pika::execution::experimental::set_value_t(MPI_Request)>;

#else
        // -----------------------------------------------------------------
        // completion signatures
        template <template <typename...> class Tuple, template <typename...> class Variant>
        using value_types = Variant<Tuple<MPI_Request>>;

        template <template <typename...> class Variant>
        using error_types = util::detail::unique_t<util::detail::prepend_t<
            typename ex::sender_traits<Sender>::template error_types<Variant>, std::exception_ptr>>;

#endif

        static constexpr bool sends_done = false;

        // -----------------------------------------------------------------
        // operation state for an internal receiver
        template <typename Receiver>
        struct operation_state
        {
            std::decay_t<Receiver> receiver;
            std::decay_t<F> f;
            stream_type stream_;

            // TODO: We should use the condition variable and mutex from the trigger operation
            // state, not duplicate them here.
            bool completed = false;
            pika::detail::spinlock mutex;
            pika::condition_variable cond_var;

            // -----------------------------------------------------------------
            // The mpi_receiver receives inputs from the previous sender,
            // invokes the mpi call, and sets a callback on the polling handler
            struct dispatch_mpi_receiver
            {
                using is_receiver = void;

                operation_state& op_state;

                template <typename Error>
                friend constexpr void
                tag_invoke(ex::set_error_t, dispatch_mpi_receiver r, Error&& error) noexcept
                {
                    ex::set_error(PIKA_MOVE(r.op_state.receiver), PIKA_FORWARD(Error, error));
                }

                friend constexpr void tag_invoke(
                    ex::set_stopped_t, dispatch_mpi_receiver r) noexcept
                {
                    ex::set_stopped(PIKA_MOVE(r.op_state.receiver));
                }

                /// typedef int (MPIX_Continue_cb_function)(int rc, void *cb_data);
                static int cb([[maybe_unused]] int rc, void* cb_data)
                {
                    std::cerr << "in continuation\n";

                    auto& op_state = *static_cast<operation_state*>(cb_data);

                    {
                        std::lock_guard lk(op_state.mutex);
                        op_state.completed = true;
                    }

                    op_state.cond_var.notify_one();

                    // TODO: Return rc?
                    return MPI_SUCCESS;
                }

                // receive the MPI function invocable + arguments and add a request,
                // then invoke the mpi function with the added request
                // if the invocation gives an error, set_error
                // otherwise return the request by passing it to set_value
                template <typename... Ts,
                    typename = std::enable_if_t<is_mpi_request_invocable_v<F, Ts...>>>
                friend constexpr void
                tag_invoke(ex::set_value_t, dispatch_mpi_receiver r, Ts&&... ts) noexcept
                {
                    pika::detail::try_catch_exception_ptr(
                        [&]() mutable {
                            using namespace pika::debug::detail;
                            using invoke_result_type = mpi_request_invoke_result_t<F, Ts...>;

                            /// Initialize a continuation request.
                            /// \param flags 0 or \ref MPIX_CONT_POLL_ONLY
                            /// \param max_poll the maximum number of continuations to execute when testing
                            ///                 the continuation request for completion or MPI_UNDEFINED for
                            ///                 unlimited execution of eligible continuations
                            /// \param info info object used to further control the behavior of the continuation request.
                            ///             Currently supported:
                            ///               - mpi_continue_thread: either "all" (any thread in the process may execute callbacks)
                            ///                                      or "application" (only application threads may execute callbacks; default)
                            ///               - mpi_continue_async_signal_safe: whether the callbacks may be executed from within a signal handler
                            /// \param[out] cont_req the newly created continuation request
                            ///
                            /// OMPI_DECLSPEC int MPIX_Continue_init(int flags, int max_poll, MPI_Info info, MPI_Request *cont_req);

                            // Initialize the continuation request
                            MPI_Request cont_request;
                            MPIX_Continue_init(0, 0, MPI_INFO_NULL, &cont_request);
                            MPI_Start(&cont_request);

                            PIKA_DETAIL_DP(mpi_tran<5>,
                                debug(str<>("dispatch_mpi_recv"), "set_value_t", "stream",
                                    detail::stream_name(r.op_state.stream_)));
#ifdef PIKA_HAVE_APEX
                            apex::scoped_timer apex_post("pika::mpi::post");
#endif
                            // init a request
                            MPI_Request request;
                            int status = MPI_SUCCESS;
                            // execute the mpi function call, passing in the request object
                            if constexpr (std::is_void_v<invoke_result_type>)
                            {
                                PIKA_INVOKE(PIKA_MOVE(r.op_state.f), ts..., &request);
                            }
                            else
                            {
                                static_assert(std::is_same_v<invoke_result_type, int>);
                                status = PIKA_INVOKE(PIKA_MOVE(r.op_state.f), ts..., &request);
                            }
                            PIKA_DETAIL_DP(mpi_tran<7>,
                                debug(str<>("dispatch_mpi_recv"), "invoke mpi",
                                    detail::stream_name(r.op_state.stream_), ptr(request)));

                            PIKA_ASSERT_MSG(request != MPI_REQUEST_NULL,
                                "MPI_REQUEST_NULL returned from mpi invocation");

                            if (status != MPI_SUCCESS)
                            {
                                PIKA_DETAIL_DP(mpi_tran<5>,
                                    debug(str<>("set_error"), "status != MPI_SUCCESS",
                                        detail::error_message(status)));
                                ex::set_error(PIKA_MOVE(r.op_state.receiver),
                                    std::make_exception_ptr(mpi_exception(status)));
                                return;
                            }

                            std::cerr << "registering continuation\n";
                            MPIX_Continue(
                                &request, &cb, &r.op_state, 0, MPI_STATUSES_IGNORE, cont_request);

                            {
                                std::cerr << "waiting for continuation request\n";
                                std::unique_lock l{r.op_state.mutex};
                                r.op_state.cond_var.wait(l, [&]() { return r.op_state.completed; });
                            }

                            std::cerr << "continuation notified\n";

                            MPI_Request_free(&cont_request);
                            std::cerr << "request freed\n";

                            PIKA_ASSERT(poll_request(request));

                            set_value_error_helper(
                                status, PIKA_MOVE(r.op_state.receiver), MPI_REQUEST_NULL);

                            // TODO: This is disabled only for testing of MPI continuations. The
                            // final version should use the trigger MPI receiver to wait for the
                            // request.

                            // if (poll_request(request))
                            // {
                            // #ifdef PIKA_HAVE_APEX
                            //                             apex::scoped_timer apex_invoke("pika::mpi::trigger");
                            // #endif
                            //     PIKA_DETAIL_DP(mpi_tran<7>,
                            //         debug(str<>("dispatch_mpi_recv"), "eager poll ok",
                            //             detail::stream_name(r.op_state.stream_), ptr(request)));
                            //     // calls set_value(request), or set_error(mpi_exception(status))
                            //     set_value_error_helper(
                            //         status, PIKA_MOVE(r.op_state.receiver), MPI_REQUEST_NULL);
                            // }
                            // else
                            // {
                            //     set_value_error_helper(
                            //         status, PIKA_MOVE(r.op_state.receiver), request);
                            // }
                        },
                        [&](std::exception_ptr ep) {
                            ex::set_error(PIKA_MOVE(r.op_state.receiver), PIKA_MOVE(ep));
                        });
                }

                friend constexpr ex::empty_env tag_invoke(
                    ex::get_env_t, dispatch_mpi_receiver const&) noexcept
                {
                    return {};
                }
            };

            using operation_state_type =
                ex::connect_result_t<std::decay_t<Sender>, dispatch_mpi_receiver>;
            operation_state_type op_state;

            template <typename Tuple>
            struct value_types_helper
            {
                using type = util::detail::transform_t<Tuple, std::decay>;
            };

            template <typename Receiver_, typename F_, typename Sender_>
            operation_state(Receiver_&& receiver, F_&& f, Sender_&& sender, stream_type s)
              : receiver(PIKA_FORWARD(Receiver_, receiver))
              , f(PIKA_FORWARD(F_, f))
              , stream_{s}
              , op_state(ex::connect(PIKA_FORWARD(Sender_, sender), dispatch_mpi_receiver{*this}))
            {
                using namespace pika::debug::detail;
                PIKA_DETAIL_DP(
                    mpi_tran<5>, debug(str<>("operation_state"), "stream", detail::stream_name(s)));
            }

            friend constexpr auto tag_invoke(ex::start_t, operation_state& os) noexcept
            {
                return ex::start(os.op_state);
            }
        };

        template <typename Receiver>
        friend constexpr auto
        tag_invoke(ex::connect_t, dispatch_mpi_sender_type const& s, Receiver&& receiver)
        {
            return operation_state<Receiver>(
                PIKA_FORWARD(Receiver, receiver), s.f, s.sender, s.stream);
        }

        template <typename Receiver>
        friend constexpr auto
        tag_invoke(ex::connect_t, dispatch_mpi_sender_type&& s, Receiver&& receiver)
        {
            return operation_state<Receiver>(
                PIKA_FORWARD(Receiver, receiver), PIKA_MOVE(s.f), PIKA_MOVE(s.sender), s.stream);
        }
    };

}    // namespace pika::mpi::experimental::detail

namespace pika::mpi::experimental {
    inline constexpr struct dispatch_mpi_t final
      : pika::functional::detail::tag_fallback<dispatch_mpi_t>
    {
    private:
        template <typename Sender, typename F,
            PIKA_CONCEPT_REQUIRES_(
                pika::execution::experimental::is_sender_v<std::decay_t<Sender>>)>
        friend constexpr PIKA_FORCEINLINE auto
        tag_fallback_invoke(dispatch_mpi_t, Sender&& sender, F&& f, stream_type s)
        {
            auto snd1 = detail::dispatch_mpi_sender<Sender, F>{
                PIKA_FORWARD(Sender, sender), PIKA_FORWARD(F, f), s};
            return pika::execution::experimental::make_unique_any_sender(std::move(snd1));
        }

        template <typename F>
        friend constexpr PIKA_FORCEINLINE auto
        tag_fallback_invoke(dispatch_mpi_t, F&& f, stream_type s)
        {
            return pika::execution::experimental::detail::partial_algorithm<dispatch_mpi_t, F>{
                PIKA_FORWARD(F, f), s};
        }

    } dispatch_mpi{};

}    // namespace pika::mpi::experimental
