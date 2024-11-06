//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////
// The purpose of this example is to execute a pika-thread printing
// "Hello World!" once. That's all.

#include <pika/async_rw_mutex.hpp>
#include <pika/execution.hpp>
#include <pika/init.hpp>
#include <pika/testing.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>

namespace pika {
    std::atomic<int> random_value = 0;
}

namespace ex = pika::execution::experimental;
namespace tt = pika::this_thread::experimental;

int pika_main()
{
    pika::scoped_finalize sf{};

    ex::thread_pool_scheduler sched{};

    ex::async_rw_mutex<int> m{42};

    for (std::size_t iteration = 0; iteration != 1; ++iteration)
    {
        PIKA_LOG(warn, "in pika_main");
        ex::start_detached(m.readwrite() | ex::continues_on(sched) | ex::then([](auto&&) {
            PIKA_LOG(warn, "in first continuation, sleeping");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            PIKA_LOG(warn, "in first continuation, continuing");
        }));
        PIKA_LOG(warn, "detached first access, waiting for second");
        tt::sync_wait(m.readwrite());
        PIKA_LOG(warn, "trying to get non-yielding access");
        auto t_1 = pika::get_worker_thread_num();
        pika::random_value = 1;
        auto x = tt::sync_wait(m.read());
        pika::random_value = 0;
        auto t_2 = pika::get_worker_thread_num();

        if (t_1 != t_2)
        {
            PIKA_LOG(critical,
                "thread number changed during read operation (before: {}, after: {})", t_1, t_2);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) { return pika::init(pika_main, argc, argv); }
