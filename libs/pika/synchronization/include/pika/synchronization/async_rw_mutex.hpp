//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/allocator_support/internal_allocator.hpp>
#include <pika/assert.hpp>
#include <pika/execution_base/operation_state.hpp>
#include <pika/execution_base/receiver.hpp>
#include <pika/execution_base/sender.hpp>
#include <pika/logging.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

// See the doxygen documentation details below for usage information. Implementation details are
// provided here.
//
// At a high level, the async_rw_mutex protects access to a given resource using a reference-counted
// shared state per access (consecutive read-only accesses share the shared state to allow
// concurrent access).
//
// Because the order of completion of senders depends on the order in which the senders are
// accessed, shared states must be allocated eagerly on creation of a sender, instead of e.g.
// relying on the operation state for storage. Until a the senders are connected and started to
// create the operation state we don't yet have a stable storage to use for linking together the
// different accesses in the correct order, so we allocate the shared state eagerly to keep track of
// the order of accesses.
//
// The flow from accessing a sender to releasing the next access is as follows:
// - A sender is created along with a new shared state which keeps track of the next access.
//   - If a consecutive read-only access is done, the shared state is simply reused from the
//     previous access.
// - If there was a previous access (this happens every time except for on the first access), we set
//   the newly allocated shared state as the next state on the previous shared state (using
//   set_next_state).
// - If the wrapped type is not void, we also tell the newly allocated shared state where to find
//   the value. The value is stored in a separate shared_ptr, and is reference counted by all the
//   shared states. When the last shared state is released, the value is also released.
// - When the sender is connected to a receiver and the operation state started, the shared state
//   must now keep track of whether we can give access to the wrapped value (i.e. signal set_value).
//   - If the previous access is done, it will have been signaled by the previous shared state by
//     calling the done method from the previous shared state on the next shared state. This is
//     called in the destructor of the previous shared state.
//   - If the previous access is done, we can call set_value immediately, passing on a reference of
//     the shared state inside a wrapper type, so that the receiver can decide when the current
//     access is done.
//   - If the previous access isn't done, we add the operation state to an intrusive queue. The head
//     of the queue is stored in the shared state. The head of the queue is initially nullptr. When
//     the queue has been processed (i.e. done has been called), the queue head points to the shared
//     state itself. Otherwise the queue points to a valid operation state.
//   - If the queue is not empty when done is called, done will traverse the queue, calling the
//     continuation method on the operation state. The continuation method will signal the receiver
//     associated with the operation state.
// - When all receivers are done accessing the shared state, and the reference count goes to the
//   zero, the shared state is destroyed. The destructor calls done on the next shared state, if one
//   has been set, and the chain of accesses continues the same way.
//
// Additional design notes:
// - If non-void, the value is stored in a separate shared state, rather than in each shared state
//   individually. This requires one extra allocation, but avoids having to move the value between
//   shared states.
// - The intrusive queue of operation states is LIFO, since we only keep track of the head of the
//   queue and push operation states to the front. When processing the queue we have to start from
//   the latest operation state. Continuations will be triggered in reverse order from how they were
//   started (not the order the senders were created).
// - The intrusive queue uses void* because there is a special value to mark that the queue has been
//   processed: the `this` pointer. This is not a pointer to an operation state, and should never be
//   dereferenced. All other non-nullptr values in the queue must be operation states and can be
//   cast to the correct type.
//
// After one read-write access, three read-only accesses, and one more read-write accesses, the
// senders (s_i_j, with i the index of the shared state and j the index of the access within that
// shared state) would point to shared states (sh_st_i) as follows:
//
// ┌───────┐    ┌───────┐    ┌───────┐
// │sh_st_1├───►│sh_st_2├───►│sh_st_3│
// └───▲───┘    └───▲▲▲─┘    └───▲───┘
//     │            │││          │
//  ┌──┴──┐      ┌──┴┴┴┐      ┌──┴──┐
//  │s_1_1│      │s_2_1├┐     │s_3_1│
//  └─────┘      └┬────┘├┐    └─────┘
//                └┬────┘│
//                 └─────┘
//
// Once all senders have been connected and the associated operation states started, the shared
// states and operation states (op_i_j corresponds to s_i_j) may relate as follows:
//
//               ┌──────┐
//               │op_2_1│
//               └──▲───┘
//               ┌──┴───┐
//               │op_2_2│
//               └──▲───┘
//  ┌──────┐     ┌──┴───┐     ┌──────┐
//  │op_1_1│     │op_2_3│     │op_3_1│
//  └──▲───┘     └──▲───┘     └──▲───┘
//     │            │            │
// ┌───┴───┐    ┌───┴───┐    ┌───┴───┐
// │sh_st_1│───►│sh_st_2│───►│sh_st_3│
// └───────┘    └───────┘    └───────┘

namespace pika::execution::experimental {
    /// \brief The type of access provided by async_rw_mutex.
    enum class async_rw_mutex_access_type
    {
        /// \brief Read-only access.
        read,
        /// \brief Read-write access.
        readwrite
    };

    /// \brief A wrapper for values sent by senders from \ref async_rw_mutex.
    ///
    /// All values sent by senders accessed through \ref async_rw_mutex are wrapped by this class.
    /// The wrapper has reference semantics to the wrapped object, and controls when subsequent
    /// accesses is given. When the destructor of the last or only wrapper runs, senders for
    /// subsequent accesses will signal their value channel.
    ///
    /// When the access type is \ref async_rw_mutex_access_type::readwrite the wrapper is move-only.
    /// When the access type is \ref async_rw_mutex_access_type::read the wrapper is copyable.
    template <typename ReadWriteT, typename ReadT, async_rw_mutex_access_type AccessType>
    class async_rw_mutex_access_wrapper;
}    // namespace pika::execution::experimental

namespace pika::async_rw_mutex_detail {
    // Helper base class to control whether a type is copyable or not. Useful to make sure e.g.
    // read-write classes are move-only and read-only classes are copyable.
    template <pika::execution::experimental::async_rw_mutex_access_type AccessType>
    struct copyability;

    template <>
    struct copyability<pika::execution::experimental::async_rw_mutex_access_type::read>
    {
        copyability() noexcept = default;
        copyability(copyability&&) noexcept = default;
        copyability& operator=(copyability&&) noexcept = default;
        copyability(copyability const&) noexcept = default;
        copyability& operator=(copyability const&) noexcept = default;
    };

    template <>
    struct copyability<pika::execution::experimental::async_rw_mutex_access_type::readwrite>
    {
        copyability() noexcept = default;
        copyability(copyability&&) noexcept = default;
        copyability& operator=(copyability&&) noexcept = default;
        copyability(copyability const&) = delete;
        copyability& operator=(copyability const&) = delete;
    };

    struct operation_state_base
    {
        // This is most of the time an operation_state_base*, but can also contain the special value
        // of the address of the shared state, hence this is a void*.
        void* next{nullptr};
        virtual void continuation() noexcept = 0;
    };

    struct shared_state_base
    {
        using shared_state_ptr_type = std::shared_ptr<shared_state_base>;
        shared_state_ptr_type next_state{nullptr};
        std::atomic<void*> op_state_head{nullptr};

        shared_state_base() = default;
        shared_state_base(shared_state_base&&) = delete;
        shared_state_base& operator=(shared_state_base&&) = delete;
        shared_state_base(shared_state_base const&) = delete;
        shared_state_base& operator=(shared_state_base const&) = delete;

        virtual ~shared_state_base()
        {
            if (next_state)
            {
                // We are also not accessing this shared state directly anymore, so we reset
                // the next_state before calling done to avoid continuations being triggered by
                // this reference being the last reference (if done after swapping the head of
                // the queue that happens in done). When resetting before the swap of the head
                // of the queue, we also know this can't be the last reference since senders
                // that reference the shared state can't be used without adding a continuation
                // to the queue (a continuation will hold another reference to the shared
                // state). Continuations can run inline, but that can only happen after the head
                // of the queue has been swapped. In summary, there must be at least two
                // references to the shared state at this point, so we can safely reset it early.
                shared_state_base* p = next_state.get();

                PIKA_ASSERT(next_state.use_count() > 1);
                next_state.reset();

                p->done();
            }
        }

        void set_next_state(shared_state_ptr_type state) noexcept
        {
            // The next state should only be set once
            PIKA_ASSERT(!next_state);
            PIKA_ASSERT(state);
            next_state = std::move(state);
        }

        bool add_op_state(operation_state_base* op_state) noexcept
        {
            op_state->next =
                static_cast<operation_state_base*>(op_state_head.load(std::memory_order_relaxed));
            do {
                if (op_state->next == static_cast<void*>(this)) { return false; }
            } while (
                !op_state_head.compare_exchange_weak(op_state->next, static_cast<void*>(op_state),
                    std::memory_order_release, std::memory_order_relaxed));

            return true;
        }

        void done_recurse(operation_state_base* current) noexcept
        {
            if (current == nullptr) { return; }
            done_recurse(static_cast<operation_state_base*>(current->next));
            current->continuation();
        }

        void done() noexcept
        {
            // `this` is not an operation_state_base*, but is a known value to signal that the queue
            // has been processed
            auto* current = static_cast<operation_state_base*>(
                op_state_head.exchange(static_cast<void*>(this), std::memory_order_acquire));

            // We have now successfully acquired the head of the queue, and signaled to other
            // threads that they can't add any more items to the queue. We can now process the
            // queue without further synchronization.

            // Because of the way operation states are linked together, they will be accessed in
            // LIFO order (op_state_head points to the last operation state to be added, or
            // nullptr). This can be surprising, so we recurse through the list and call the
            // continuation on the way back up from the recursion, resulting in FIFO order for
            // the continuations.
            //
            // We will not guarantee this behaviour, but it is a nice property to have.
            done_recurse(current);
        }
    };

    template <typename T>
    struct shared_state : shared_state_base
    {
        using value_ptr_type = std::shared_ptr<T>;
        value_ptr_type value{nullptr};

        shared_state(value_ptr_type value)
          : value(std::move(value))
        {
        }

        T& get_value() noexcept
        {
            PIKA_ASSERT(value);
            return *value;
        }
    };

    template <>
    struct shared_state<void> : shared_state_base
    {
    };

    template <typename ReadWriteT, typename ReadT,
        pika::execution::experimental::async_rw_mutex_access_type AccessType, typename R>
    struct operation_state : operation_state_base
    {
        using access_type = pika::execution::experimental::async_rw_mutex_access_wrapper<ReadWriteT,
            ReadT, AccessType>;
        using shared_state_type = shared_state<ReadWriteT>;
        using shared_state_ptr_type = std::shared_ptr<shared_state_type>;

        std::decay_t<R> r;
        shared_state_ptr_type state;

        template <typename R_>
        operation_state(R_&& r, shared_state_ptr_type state)
          : r(std::forward<R_>(r))
          , state(std::move(state))
        {
        }

        operation_state(operation_state&&) = delete;
        operation_state& operator=(operation_state&&) = delete;
        operation_state(operation_state const&) = delete;
        operation_state& operator=(operation_state const&) = delete;

        ~operation_state() noexcept
        {
            if (state)
            {
                PIKA_LOG(err,
                    "async_rw_mutex operation state was destroyed without the shared state "
                    "being released, was the operation state never started?");
                std::terminate();
            }
        }

        void continuation() noexcept override
        {
            try
            {
                pika::execution::experimental::set_value(
                    std::move(r), access_type{std::move(state)});
            }
            catch (...)
            {
                state.reset();
                pika::execution::experimental::set_error(std::move(r), std::current_exception());
            }
        }

        friend void tag_invoke(pika::execution::experimental::start_t, operation_state& os) noexcept
        {
            PIKA_ASSERT_MSG(os.state,
                "async_rw_mutex operation state shared state is empty, was the sender already "
                "started?");

            if (!os.state->add_op_state(&os))
            {
                // The previous shared state has already been released. We can run the continuation
                // immediately.
                os.continuation();
            }
        }
    };

    template <typename ReadWriteT, typename ReadT,
        pika::execution::experimental::async_rw_mutex_access_type AccessType>
    struct sender : private copyability<AccessType>
    {
        PIKA_STDEXEC_SENDER_CONCEPT

        using shared_state_type = shared_state<ReadWriteT>;
        using shared_state_ptr_type = std::shared_ptr<shared_state_type>;

        shared_state_ptr_type state;

        using access_type = pika::execution::experimental::async_rw_mutex_access_wrapper<ReadWriteT,
            ReadT, AccessType>;
        template <template <typename...> class Tuple, template <typename...> class Variant>
        using value_types = Variant<Tuple<access_type>>;

        template <template <typename...> class Variant>
        using error_types = Variant<std::exception_ptr>;

        static constexpr bool sends_done = false;

        using completion_signatures = pika::execution::experimental::completion_signatures<
            pika::execution::experimental::set_value_t(access_type),
            pika::execution::experimental::set_error_t(std::exception_ptr)>;

        template <typename Receiver>
        using operation_state_type = operation_state<ReadWriteT, ReadT, AccessType, Receiver>;

        explicit sender(shared_state_ptr_type state) noexcept
          : state(std::move(state))
        {
        }

        sender(sender&&) noexcept = default;
        sender& operator=(sender&&) noexcept = default;
        sender(sender const&) noexcept = default;
        sender& operator=(sender const&) noexcept = default;

        ~sender() noexcept
        {
            if (state)
            {
                PIKA_LOG(err,
                    "async_rw_mutex sender was destroyed without the shared state being "
                    "released, was the sender never connected?");
                std::terminate();
            }
        }

        template <typename Receiver>
        friend operation_state_type<Receiver>
        tag_invoke(pika::execution::experimental::connect_t, sender&& s, Receiver&& r)
        {
            return operation_state_type<Receiver>{std::forward<Receiver>(r), std::move(s.state)};
        }

        template <typename Receiver>
        friend auto
        tag_invoke(pika::execution::experimental::connect_t, sender const& s, Receiver&& r)
        {
            if constexpr (AccessType ==
                pika::execution::experimental::async_rw_mutex_access_type::readwrite)
            {
                static_assert(sizeof(Receiver) == 0,
                    "Are you missing a std::move? The async_rw_mutex sender in read-write mode "
                    "is not copyable and thus not l-value connectable. Make sure you are "
                    "passing a non-const r-value reference of the sender or accessing the "
                    "sender in the correct mode.");
            }

            return operation_state_type<Receiver>{std::forward<Receiver>(r), s.state};
        }
    };

    template <typename ReadWriteT, typename ReadT, typename Allocator>
    class async_rw_mutex_base
    {
    protected:
        using shared_state_type = shared_state<ReadWriteT>;
        using shared_state_ptr_type = std::shared_ptr<shared_state_type>;

        template <pika::execution::experimental::async_rw_mutex_access_type AccessType>
        using sender_type = sender<ReadWriteT, ReadT, AccessType>;

    public:
        using read_type = ReadT;
        using readwrite_type = ReadWriteT;
        using read_access_type =
            pika::execution::experimental::async_rw_mutex_access_wrapper<readwrite_type, read_type,
                pika::execution::experimental::async_rw_mutex_access_type::read>;
        using readwrite_access_type =
            pika::execution::experimental::async_rw_mutex_access_wrapper<readwrite_type, read_type,
                pika::execution::experimental::async_rw_mutex_access_type::readwrite>;
        using read_sender_type =
            sender_type<pika::execution::experimental::async_rw_mutex_access_type::read>;
        using readwrite_sender_type =
            sender_type<pika::execution::experimental::async_rw_mutex_access_type::readwrite>;
        using allocator_type = std::decay_t<Allocator>;

        async_rw_mutex_base() = delete;
        explicit async_rw_mutex_base(allocator_type alloc)
          : alloc(std::move(alloc))
        {
        }
        async_rw_mutex_base(async_rw_mutex_base&&) noexcept = default;
        async_rw_mutex_base& operator=(async_rw_mutex_base&&) noexcept = default;
        async_rw_mutex_base(async_rw_mutex_base const&) = delete;
        async_rw_mutex_base& operator=(async_rw_mutex_base const&) = delete;

    protected:
        PIKA_NO_UNIQUE_ADDRESS allocator_type alloc;
        shared_state_ptr_type state;
        pika::execution::experimental::async_rw_mutex_access_type prev_access =
            pika::execution::experimental::async_rw_mutex_access_type::readwrite;

        template <typename... Ts>
        read_sender_type read(Ts&... ts)
        {
            if (prev_access == pika::execution::experimental::async_rw_mutex_access_type::readwrite)
            {
                auto prev_state = std::move(state);
                state = std::allocate_shared<shared_state_type, allocator_type>(alloc, ts...);
                prev_access = pika::execution::experimental::async_rw_mutex_access_type::read;

                // Only the first access has no previous shared state.
                if (PIKA_LIKELY(prev_state)) { prev_state->set_next_state(state); }
                else { state->done(); }
            }

            return read_sender_type{state};
        }

        template <typename... Ts>
        readwrite_sender_type readwrite(Ts... ts)
        {
            auto prev_state = std::move(state);
            state = std::allocate_shared<shared_state_type, allocator_type>(alloc, ts...);
            prev_access = pika::execution::experimental::async_rw_mutex_access_type::readwrite;

            // Only the first access has no previous shared state.
            if (PIKA_LIKELY(prev_state)) { prev_state->set_next_state(state); }
            else { state->done(); }

            return readwrite_sender_type{state};
        }
    };
}    // namespace pika::async_rw_mutex_detail

namespace pika::execution::experimental {
    /// \brief A wrapper for values sent by senders from \ref async_rw_mutex with read-only access.
    ///
    /// The wrapper is copyable.
    template <typename ReadWriteT, typename ReadT>
    class async_rw_mutex_access_wrapper<ReadWriteT, ReadT, async_rw_mutex_access_type::read>
    {
    private:
        using shared_state_type =
            std::shared_ptr<pika::async_rw_mutex_detail::shared_state<ReadWriteT>>;
        shared_state_type state;

    public:
        async_rw_mutex_access_wrapper() = delete;
        explicit async_rw_mutex_access_wrapper(shared_state_type state) noexcept
          : state(std::move(state))
        {
        }
        async_rw_mutex_access_wrapper(async_rw_mutex_access_wrapper&&) noexcept = default;
        async_rw_mutex_access_wrapper& operator=(
            async_rw_mutex_access_wrapper&&) noexcept = default;
        async_rw_mutex_access_wrapper(async_rw_mutex_access_wrapper const&) noexcept = default;
        async_rw_mutex_access_wrapper& operator=(
            async_rw_mutex_access_wrapper const&) noexcept = default;

        /// \brief Access the wrapped type by const reference.
        ReadT& get() const noexcept
        {
            PIKA_ASSERT(state);
            return state->get_value();
        }
    };

    /// \brief A wrapper for values sent by senders from \ref async_rw_mutex with read-write access.
    ///
    /// The wrapper is move-only.
    template <typename ReadWriteT, typename ReadT>
    class async_rw_mutex_access_wrapper<ReadWriteT, ReadT, async_rw_mutex_access_type::readwrite>
    {
    private:
        static_assert(!std::is_void<ReadWriteT>::value,
            "Cannot mix void and non-void type in async_rw_mutex_access_wrapper wrapper "
            "(ReadWriteT is void, ReadT is non-void)");
        static_assert(!std::is_void<ReadT>::value,
            "Cannot mix void and non-void type in async_rw_mutex_access_wrapper wrapper (ReadT "
            "is void, ReadWriteT is non-void)");

        using shared_state_type =
            std::shared_ptr<pika::async_rw_mutex_detail::shared_state<ReadWriteT>>;
        shared_state_type state;

    public:
        async_rw_mutex_access_wrapper() = delete;
        explicit async_rw_mutex_access_wrapper(shared_state_type state) noexcept
          : state(std::move(state))
        {
        }
        async_rw_mutex_access_wrapper(async_rw_mutex_access_wrapper&&) noexcept = default;
        async_rw_mutex_access_wrapper& operator=(
            async_rw_mutex_access_wrapper&&) noexcept = default;
        async_rw_mutex_access_wrapper(async_rw_mutex_access_wrapper const&) = delete;
        async_rw_mutex_access_wrapper& operator=(async_rw_mutex_access_wrapper const&) = delete;

        /// \brief Access the wrapped type by reference.
        ReadWriteT& get()
        {
            PIKA_ASSERT(state);
            return state->get_value();
        }
    };

    // The void wrappers for read and readwrite are identical, but must be
    // specialized separately to avoid ambiguity with the non-void
    // specializations above.

    /// \brief A wrapper for read-only access granted by a \p void \ref async_rw_mutex.
    ///
    /// The wrapper is copyable.
    template <>
    class async_rw_mutex_access_wrapper<void, void, async_rw_mutex_access_type::read>
    {
    private:
        using shared_state_type = std::shared_ptr<pika::async_rw_mutex_detail::shared_state<void>>;
        shared_state_type state;

    public:
        async_rw_mutex_access_wrapper() = delete;
        explicit async_rw_mutex_access_wrapper(shared_state_type state) noexcept
          : state(std::move(state))
        {
        }
        async_rw_mutex_access_wrapper(async_rw_mutex_access_wrapper&&) noexcept = default;
        async_rw_mutex_access_wrapper& operator=(
            async_rw_mutex_access_wrapper&&) noexcept = default;
        async_rw_mutex_access_wrapper(async_rw_mutex_access_wrapper const&) noexcept = default;
        async_rw_mutex_access_wrapper& operator=(
            async_rw_mutex_access_wrapper const&) noexcept = default;
    };

    /// \brief A wrapper for read-write access granted by a \p void \ref async_rw_mutex.
    ///
    /// The wrapper is move-only.
    template <>
    class async_rw_mutex_access_wrapper<void, void, async_rw_mutex_access_type::readwrite>
    {
    private:
        using shared_state_type = std::shared_ptr<pika::async_rw_mutex_detail::shared_state<void>>;
        shared_state_type state;

    public:
        async_rw_mutex_access_wrapper() = delete;
        explicit async_rw_mutex_access_wrapper(shared_state_type state) noexcept
          : state(std::move(state))
        {
        }
        async_rw_mutex_access_wrapper(async_rw_mutex_access_wrapper&&) noexcept = default;
        async_rw_mutex_access_wrapper& operator=(
            async_rw_mutex_access_wrapper&&) noexcept = default;
        async_rw_mutex_access_wrapper(async_rw_mutex_access_wrapper const&) = delete;
        async_rw_mutex_access_wrapper& operator=(async_rw_mutex_access_wrapper const&) = delete;
    };

    /// \brief Read-write mutex where access is granted to a value through senders.
    ///
    /// The wrapped value is accessed through \ref read and \ref readwrite, both of which return
    /// senders which send a wrapped value on the value channel when the wrapped value is safe to
    /// read or write.
    ///
    /// A read-write sender gives exclusive access to the wrapped value, while a read-only sender
    /// allows concurrent access to the value (with other read-only accesses).
    ///
    /// When the wrapped type is \p void, the mutex acts as a simple mutex around some externally
    /// managed resource. The mutex still allows read-write and read-only access when the type is \p
    /// void. The read-write wrapper types are move-only. The read-only wrapper types are copyable.
    ///
    /// The order in which senders signal a receiver is determined by the order in which the senders
    /// are retrieved from the mutex. Connecting and starting the senders is thread-safe.
    ///
    /// The mutex is move-only.
    ///
    /// \warning Because access to the wrapped value is granted in the order that it is requested
    /// from the mutex, there is a risk of deadlocks if senders of later accesses are started and
    /// waited for without starting senders of earlier accesses.
    ///
    /// \warning Retrieving senders from the mutex is not thread-safe. The senders of the mutex are
    /// intended to be accessed in synchronous code, while the access provided by the senders
    /// themselves are safe to access concurrently.
    ///
    /// \tparam ReadWriteT The type of the wrapped type.
    /// \tparam ReadT The type to use for read-only accesses of the wrapped type. Defaults to \ref
    /// ReadWriteT.
    /// \tparam Allocator The allocator to use for allocating the internal shared state.
    template <typename ReadWriteT = void, typename ReadT = ReadWriteT,
        typename Allocator = pika::detail::internal_allocator<>>
    class async_rw_mutex;

    template <typename Allocator>
    class async_rw_mutex<void, void, Allocator>
      : public pika::async_rw_mutex_detail::async_rw_mutex_base<void, void, Allocator>
    {
    private:
        using base_type = pika::async_rw_mutex_detail::async_rw_mutex_base<void, void, Allocator>;
        using shared_state_type = typename base_type::shared_state_type;
        using shared_state_ptr_type = typename base_type::shared_state_ptr_type;
        template <async_rw_mutex_access_type AccessType>
        using sender_type = typename base_type::sender_type<AccessType>;

        using base_type::alloc;

    public:
        using read_type = typename base_type::read_type;
        using readwrite_type = typename base_type::readwrite_type;
        using read_access_type = typename base_type::read_access_type;
        using readwrite_access_type = typename base_type::readwrite_access_type;
        using read_sender_type = typename base_type::read_sender_type;
        using readwrite_sender_type = typename base_type::readwrite_sender_type;
        using allocator_type = typename base_type::allocator_type;

        explicit async_rw_mutex(allocator_type alloc = {})
          : base_type(std::move(alloc))
        {
        }
        async_rw_mutex(async_rw_mutex&&) noexcept = default;
        async_rw_mutex& operator=(async_rw_mutex&&) noexcept = default;
        async_rw_mutex(async_rw_mutex const&) = delete;
        async_rw_mutex& operator=(async_rw_mutex const&) = delete;

        read_sender_type read() { return base_type::read(); }
        readwrite_sender_type readwrite() { return base_type::readwrite(); }
    };

    template <typename ReadWriteT, typename ReadT, typename Allocator>
    class async_rw_mutex
      : public pika::async_rw_mutex_detail::async_rw_mutex_base<std::decay_t<ReadWriteT>,
            std::decay_t<ReadT> const, Allocator>
    {
    private:
        static_assert(!std::is_void<ReadWriteT>::value,
            "Cannot mix void and non-void type in async_rw_mutex (ReadWriteT is void, ReadT is "
            "non-void)");
        static_assert(!std::is_void<ReadT>::value,
            "Cannot mix void and non-void type in async_rw_mutex (ReadT is void, ReadWriteT is "
            "non-void)");

        using base_type = pika::async_rw_mutex_detail::async_rw_mutex_base<std::decay_t<ReadWriteT>,
            std::decay_t<ReadT> const, Allocator>;
        using shared_state_type = typename base_type::shared_state_type;
        using shared_state_ptr_type = typename base_type::shared_state_ptr_type;
        template <async_rw_mutex_access_type AccessType>
        using sender_type = typename base_type::sender_type<AccessType>;

        using base_type::alloc;

    public:
        /// \brief The type of read-only types accessed through the mutex.
        using read_type = typename base_type::read_type;

        /// \brief The type of read-write types accessed through the mutex.
        using readwrite_type = typename base_type::readwrite_type;

        /// \brief The wrapper type sent by read-only-access senders.
        using read_access_type = typename base_type::read_access_type;

        /// \brief The wrapper type sent by read-write-access senders.
        using readwrite_access_type = typename base_type::readwrite_access_type;

        /// \brief The type of read-only-access senders.
        using read_sender_type = typename base_type::read_sender_type;

        /// \brief The type of read-write-access senders.
        using readwrite_sender_type = typename base_type::readwrite_sender_type;

        using allocator_type = typename base_type::allocator_type;

    private:
        using value_ptr_type = std::shared_ptr<readwrite_type>;
        value_ptr_type value;

    public:
        async_rw_mutex() = delete;

        /// \brief Construct a new mutex with the wrapped value initialized to \p u.
        template <typename U,
            typename = std::enable_if_t<!std::is_same<std::decay_t<U>, async_rw_mutex>::value>>
        explicit async_rw_mutex(U&& u, allocator_type alloc = {})
          : base_type(std::move(alloc))
          , value(std::allocate_shared<readwrite_type, allocator_type>(
                this->alloc, std::forward<U>(u)))
        {
        }
        async_rw_mutex(async_rw_mutex&&) noexcept = default;
        async_rw_mutex& operator=(async_rw_mutex&&) noexcept = default;
        async_rw_mutex(async_rw_mutex const&) = delete;
        async_rw_mutex& operator=(async_rw_mutex const&) = delete;

        /// \brief Destroy the mutex.
        ///
        /// The destructor does not wait or require that all accesses through senders have
        /// completed. The wrapped value is kept alive in a shared state managed by the senders,
        /// until the last access completes, or the destructor of the \ref async_rw_mutex runs,
        /// whichever happens later.
        ~async_rw_mutex() = default;

        /// \brief Access the wrapped value in read-only mode through a sender.
        read_sender_type read() { return base_type::read(value); }

        /// \brief Access the wrapped value in read-write mode through a sender.
        readwrite_sender_type readwrite() { return base_type::readwrite(value); }
    };
}    // namespace pika::execution::experimental
