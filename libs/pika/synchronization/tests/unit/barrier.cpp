//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/barrier.hpp>
#include <pika/execution.hpp>
#include <pika/init.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

std::atomic<std::size_t> c1(0);
std::atomic<std::size_t> c2(0);

namespace ex = pika::execution::experimental;
namespace tt = pika::this_thread::experimental;

///////////////////////////////////////////////////////////////////////////////
void local_barrier_test_no_completion(pika::barrier<>& b)
{
    ++c1;

    // wait for all threads to enter the barrier
    b.arrive_and_wait();

    ++c2;
}

void test_barrier_empty_oncomplete()
{
    constexpr std::size_t threads = 64;
    constexpr std::size_t iterations = 100;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        // create a barrier waiting on 'count' threads
        pika::barrier<> b(threads + 1);
        c1 = 0;
        c2 = 0;

        // create the threads which will wait on the barrier
        std::vector<ex::unique_any_sender<>> results;
        results.reserve(threads);
        for (std::size_t i = 0; i != threads; ++i)
        {
            results.emplace_back(ex::transfer_just(ex::thread_pool_scheduler{}, std::ref(b)) |
                ex::then(local_barrier_test_no_completion) | ex::ensure_started());
        }

        b.arrive_and_wait();    // wait for all threads to enter the barrier
        PIKA_TEST_EQ(threads, c1.load());

        tt::sync_wait(ex::when_all_vector(std::move(results)));

        PIKA_TEST_EQ(threads, c2.load());
    }
}

///////////////////////////////////////////////////////////////////////////////
std::atomic<std::size_t> complete(0);

struct oncomplete
{
    void operator()() const { ++complete; }
};

void local_barrier_test(pika::barrier<oncomplete>& b)
{
    ++c1;

    // wait for all threads to enter the barrier
    b.arrive_and_wait();

    ++c2;
}

void test_barrier_oncomplete()
{
    constexpr std::size_t threads = 64;
    constexpr std::size_t iterations = 100;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        // create a barrier waiting on 'count' threads
        pika::barrier<oncomplete> b(threads + 1);
        c1 = 0;
        c2 = 0;
        complete = 0;

        // create the threads which will wait on the barrier
        std::vector<ex::unique_any_sender<>> results;
        results.reserve(threads);
        for (std::size_t i = 0; i != threads; ++i)
        {
            results.emplace_back(ex::transfer_just(ex::thread_pool_scheduler{}, std::ref(b)) |
                ex::then(local_barrier_test) | ex::ensure_started());
        }

        b.arrive_and_wait();    // wait for all threads to enter the barrier
        PIKA_TEST_EQ(threads, c1.load());

        tt::sync_wait(ex::when_all_vector(std::move(results)));

        PIKA_TEST_EQ(threads, c2.load());
        PIKA_TEST_EQ(complete.load(), std::size_t(1));
    }
}

///////////////////////////////////////////////////////////////////////////////
void local_barrier_test_no_completion_split(pika::barrier<>& b)
{
    ++c1;

    // signal the barrier
    auto token = b.arrive();

    // wait for all threads to enter the barrier
    b.wait(std::move(token));

    ++c2;
}

void test_barrier_empty_oncomplete_split()
{
    constexpr std::size_t threads = 64;
    constexpr std::size_t iterations = 100;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        // create a barrier waiting on 'count' threads
        pika::barrier<> b(threads + 1);
        c1 = 0;
        c2 = 0;

        // create the threads which will wait on the barrier
        std::vector<ex::unique_any_sender<>> results;
        results.reserve(threads);
        for (std::size_t i = 0; i != threads; ++i)
        {
            results.emplace_back(ex::transfer_just(ex::thread_pool_scheduler{}, std::ref(b)) |
                ex::then(local_barrier_test_no_completion_split) | ex::ensure_started());
        }

        b.arrive_and_wait();    // wait for all threads to enter the barrier
        PIKA_TEST_EQ(threads, c1.load());

        tt::sync_wait(ex::when_all_vector(std::move(results)));

        PIKA_TEST_EQ(threads, c2.load());
    }
}

void local_barrier_test_split(pika::barrier<oncomplete>& b)
{
    ++c1;

    // signal the barrier
    auto token = b.arrive();

    // wait for all threads to enter the barrier
    b.wait(std::move(token));

    ++c2;
}

void test_barrier_oncomplete_split()
{
    constexpr std::size_t threads = 64;
    constexpr std::size_t iterations = 100;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        // create a barrier waiting on 'count' threads
        pika::barrier<oncomplete> b(threads + 1);
        c1 = 0;
        c2 = 0;
        complete = 0;

        // create the threads which will wait on the barrier
        std::vector<ex::unique_any_sender<>> results;
        results.reserve(threads);
        for (std::size_t i = 0; i != threads; ++i)
        {
            results.emplace_back(ex::transfer_just(ex::thread_pool_scheduler{}, std::ref(b)) |
                ex::then(local_barrier_test_split) | ex::ensure_started());
        }

        b.arrive_and_wait();    // wait for all threads to enter the barrier
        PIKA_TEST_EQ(threads, c1.load());

        tt::sync_wait(ex::when_all_vector(std::move(results)));

        PIKA_TEST_EQ(threads, c2.load());
        PIKA_TEST_EQ(complete.load(), std::size_t(1));
    }
}

///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    test_barrier_empty_oncomplete();
    test_barrier_oncomplete();

    test_barrier_empty_oncomplete_split();
    test_barrier_oncomplete_split();

    return pika::finalize();
}

int main(int argc, char* argv[]) { return pika::init(pika_main, argc, argv); }
