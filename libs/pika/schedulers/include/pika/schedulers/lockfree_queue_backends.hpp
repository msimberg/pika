////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2012 Bryce Adelstein-Lelbach
//  Copyright (c) 2019 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <pika/config.hpp>

#if defined(PIKA_HAVE_CXX11_STD_ATOMIC_128BIT)
# include <pika/concurrency/deque.hpp>
#else
# include <boost/lockfree/queue.hpp>
#endif

#include <xenium/ramalhete_queue.hpp>
#include <xenium/reclamation/generic_epoch_based.hpp>

#include <pika/allocator_support/aligned_allocator.hpp>

// Does not rely on CXX11_STD_ATOMIC_128BIT
#include <pika/concurrency/concurrentqueue.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace pika::threads::detail {

    struct lockfree_fifo;

    ///////////////////////////////////////////////////////////////////////////////
    // FIFO
    template <typename T>
    struct lockfree_fifo_backend
    {
#if defined(PIKA_HAVE_CXX11_STD_ATOMIC_128BIT)
        using container_type = pika::concurrency::detail::deque<T,
            pika::concurrency::detail::caching_freelist_t, pika::detail::aligned_allocator<T>>;
#else
        using container_type = boost::lockfree::queue<T, pika::detail::aligned_allocator<T>>;
#endif

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        lockfree_fifo_backend(
            size_type initial_size = 0, size_type /* num_thread */ = size_type(-1))
          : queue_(std::size_t(initial_size))
        {
        }

        bool push(const_reference val, bool /*other_end*/ = false)
        {
#if defined(PIKA_HAVE_CXX11_STD_ATOMIC_128BIT)
            return queue_.push_left(val);
#else
            return queue_.push(val);
#endif
        }

        bool push(rvalue_reference val, bool /*other_end*/ = false)
        {
#if defined(PIKA_HAVE_CXX11_STD_ATOMIC_128BIT)
            return queue_.push_left(PIKA_MOVE(val));
#else
            return queue_.push(PIKA_MOVE(val));
#endif
        }

        bool pop(reference val, bool /* steal */ = true)
        {
#if defined(PIKA_HAVE_CXX11_STD_ATOMIC_128BIT)
            return queue_.pop_right(val);
#else
            return queue_.pop(val);
#endif
        }

        bool empty()
        {
            return queue_.empty();
        }

    private:
        container_type queue_;
    };

    struct lockfree_fifo
    {
        template <typename T>
        struct apply
        {
            using type = lockfree_fifo_backend<T>;
        };
    };

    ////////////////////////////////////////////////////////////////////////////
    // MoodyCamel FIFO
    template <typename T>
    struct moodycamel_fifo_backend
    {
        using container_type = pika::concurrency::detail::ConcurrentQueue<T>;

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        moodycamel_fifo_backend(
            size_type initial_size = 0, size_type /* num_thread */ = size_type(-1))
          : queue_(std::size_t(initial_size))
        {
        }

        bool push(const_reference val, bool /*other_end*/ = false)
        {
            return queue_.enqueue(val);
        }

        bool push(rvalue_reference val, bool /*other_end*/ = false)
        {
            return queue_.enqueue(PIKA_MOVE(val));
        }

        bool pop(reference val, bool /* steal */ = true)
        {
            return queue_.try_dequeue(val);
        }

        bool empty()
        {
            return (queue_.size_approx() == 0);
        }

    private:
        container_type queue_;
    };

    struct concurrentqueue_fifo
    {
        template <typename T>
        struct apply
        {
            using type = moodycamel_fifo_backend<T>;
        };
    };

    // LIFO
#if defined(PIKA_HAVE_CXX11_STD_ATOMIC_128BIT)
    struct lockfree_lifo;

    template <typename T>
    struct lockfree_lifo_backend
    {
        using container_type = pika::concurrency::detail::deque<T,
            pika::concurrency::detail::caching_freelist_t, pika::detail::aligned_allocator<T>>;

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        lockfree_lifo_backend(
            size_type initial_size = 0, size_type /* num_thread */ = size_type(-1))
          : queue_(std::size_t(initial_size))
        {
        }

        bool push(const_reference val, bool other_end = false)
        {
            if (other_end)
                return queue_.push_right(val);
            return queue_.push_left(val);
        }

        bool push(rvalue_reference val, bool other_end = false)
        {
            if (other_end)
                return queue_.push_right(PIKA_MOVE(val));
            return queue_.push_left(PIKA_MOVE(val));
        }

        bool pop(reference val, bool /* steal */ = true)
        {
            return queue_.pop_left(val);
        }

        bool empty()
        {
            return queue_.empty();
        }

    private:
        container_type queue_;
    };

    struct lockfree_lifo
    {
        template <typename T>
        struct apply
        {
            using type = lockfree_lifo_backend<T>;
        };
    };

    ////////////////////////////////////////////////////////////////////////////
    // FIFO + stealing at opposite end.
    struct lockfree_abp_fifo;
    struct lockfree_abp_lifo;

    template <typename T>
    struct lockfree_abp_fifo_backend
    {
        using container_type = pika::concurrency::detail::deque<T,
            pika::concurrency::detail::caching_freelist_t, pika::detail::aligned_allocator<T>>;

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        lockfree_abp_fifo_backend(
            size_type initial_size = 0, size_type /* num_thread */ = size_type(-1))
          : queue_(std::size_t(initial_size))
        {
        }

        bool push(const_reference val, bool /*other_end*/ = false)
        {
            return queue_.push_left(val);
        }

        bool push(rvalue_reference val, bool /*other_end*/ = false)
        {
            return queue_.push_left(PIKA_MOVE(val));
        }

        bool pop(reference val, bool steal = true)
        {
            if (steal)
                return queue_.pop_left(val);
            return queue_.pop_right(val);
        }

        bool empty()
        {
            return queue_.empty();
        }

    private:
        container_type queue_;
    };

    struct lockfree_abp_fifo
    {
        template <typename T>
        struct apply
        {
            using type = lockfree_abp_fifo_backend<T>;
        };
    };

    ////////////////////////////////////////////////////////////////////////////
    // LIFO + stealing at opposite end.
    // E.g. ABP (Arora, Blumofe and Plaxton) queuing
    // http://dl.acm.org/citation.cfm?id=277678
    template <typename T>
    struct lockfree_abp_lifo_backend
    {
        using container_type = pika::concurrency::detail::deque<T,
            pika::concurrency::detail::caching_freelist_t, pika::detail::aligned_allocator<T>>;

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        lockfree_abp_lifo_backend(
            size_type initial_size = 0, size_type /* num_thread */ = size_type(-1))
          : queue_(std::size_t(initial_size))
        {
        }

        bool push(const_reference val, bool other_end = false)
        {
            if (other_end)
                return queue_.push_right(PIKA_MOVE(val));
            return queue_.push_left(PIKA_MOVE(val));
        }

        bool push(rvalue_reference val, bool other_end = false)
        {
            if (other_end)
                return queue_.push_right(val);
            return queue_.push_left(val);
        }

        bool pop(reference val, bool steal = true)
        {
            if (steal)
                return queue_.pop_right(val);
            return queue_.pop_left(val);
        }

        bool empty()
        {
            return queue_.empty();
        }

    private:
        container_type queue_;
    };

    struct lockfree_abp_lifo
    {
        template <typename T>
        struct apply
        {
            using type = lockfree_abp_lifo_backend<T>;
        };
    };

    template <typename T>
    struct lockfree_ramalhete_backend
    {
        using container_type = xenium::ramalhete_queue<T,
            xenium::policy::reclaimer<xenium::reclamation::generic_epoch_based<>>,
            xenium::policy::entries_per_node<2048>>;

        using value_type = T;
        using reference = T&;
        using const_reference = T const&;
        using rvalue_reference = T&&;
        using size_type = std::uint64_t;

        lockfree_ramalhete_backend(
            size_type /* initial_size */ = 0, size_type /* num_thread */ = size_type(-1))
          : queue_()
        {
        }

        bool push(const_reference val, bool /* other_end */ = false)
        {
            queue_.push(val);
            return true;
        }

        bool push(rvalue_reference val, bool /* other_end */ = false)
        {
            queue_.push(std::move(val));
            return true;
        }

        bool pop(reference val, bool /* steal */ = true)
        {
            return queue_.try_pop(val);
        }

        bool empty()
        {
            return false;
            // return queue_.empty();
        }

    private:
        container_type queue_;
    };

    struct lockfree_ramalhete
    {
        template <typename T>
        struct apply
        {
            using type = lockfree_ramalhete_backend<T>;
        };
    };

#endif    // PIKA_HAVE_CXX11_STD_ATOMIC_128BIT

}    // namespace pika::threads::detail
