//  Copyright (c) 2007-2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/chrono.hpp>
#include <pika/execution.hpp>
#include <pika/functional/bind_front.hpp>
#include <pika/init.hpp>
#include <pika/testing/performance.hpp>

#include "worker_timed.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

namespace ex = pika::execution::experimental;
namespace tt = pika::this_thread::experimental;

///////////////////////////////////////////////////////////////////////////////
std::size_t num_level_tasks = 16;
std::size_t spread = 2;
std::uint64_t delay_ns = 0;

void test_func() { worker_timed(delay_ns); }

///////////////////////////////////////////////////////////////////////////////
ex::unique_any_sender<> spawn_level(std::size_t num_tasks)
{
    std::vector<ex::unique_any_sender<>> tasks;
    tasks.reserve(num_tasks);

    // spawn sub-levels
    if (num_tasks > num_level_tasks && spread > 1)
    {
        std::size_t spawn_hierarchically = num_tasks - num_level_tasks;
        std::size_t num_sub_tasks = 0;
        if (spawn_hierarchically < num_level_tasks)
            num_sub_tasks = spawn_hierarchically;
        else
            num_sub_tasks = spawn_hierarchically / spread;

        for (std::size_t i = 0; i != spread && spawn_hierarchically != 0; ++i)
        {
            std::size_t sub_spawn = (std::min)(spawn_hierarchically, num_sub_tasks);
            spawn_hierarchically -= sub_spawn;
            num_tasks -= sub_spawn;

            tasks.emplace_back(ex::schedule(ex::thread_pool_scheduler{}) |
                ex::let_value(pika::util::detail::bind_front(spawn_level, sub_spawn)));
        }
    }

    // then spawn required number of tasks on this level
    for (std::size_t i = 0; i != num_tasks; ++i)
    {
        tasks.emplace_back(ex::schedule(ex::thread_pool_scheduler{}) | ex::then(test_func));
    }

    return ex::when_all_vector(std::move(tasks));
}

///////////////////////////////////////////////////////////////////////////////
int pika_main(pika::program_options::variables_map& vm)
{
    std::size_t num_tasks = 128;
    if (vm.count("tasks")) num_tasks = vm["tasks"].as<std::size_t>();

    double sequential_time_per_task = 0;

    {
        std::vector<ex::unique_any_sender<>> tasks;
        tasks.reserve(num_tasks);

        auto start = std::chrono::high_resolution_clock::now();

        for (std::size_t i = 0; i != num_tasks; ++i)
        {
            tasks.emplace_back(ex::schedule(ex::thread_pool_scheduler{}) | ex::then(test_func));
        }

        tt::sync_wait(ex::when_all_vector(std::move(tasks)));

        auto end = std::chrono::high_resolution_clock::now();

        sequential_time_per_task = std::chrono::duration<double>(end - start).count() / num_tasks;
        std::cout << "Elapsed sequential time: "
                  << std::chrono::duration<double>(end - start).count() << " [s], ("
                  << sequential_time_per_task << " [s])" << std::endl;
        pika::util::print_cdash_timing("AsyncSequential", sequential_time_per_task);
    }

    double hierarchical_time_per_task = 0;

    {
        auto start = std::chrono::high_resolution_clock::now();

        tt::sync_wait(
            ex::transfer_just(ex::thread_pool_scheduler{}, num_tasks) | ex::then(spawn_level));

        auto end = std::chrono::high_resolution_clock::now();

        hierarchical_time_per_task = std::chrono::duration<double>(end - start).count() / num_tasks;
        std::cout << "Elapsed hierarchical time: "
                  << std::chrono::duration<double>(end - start).count() << " [s], ("
                  << hierarchical_time_per_task << " [s])" << std::endl;
        pika::util::print_cdash_timing("AsyncHierarchical", hierarchical_time_per_task);
    }

    std::cout << "Ratio (speedup): " << sequential_time_per_task / hierarchical_time_per_task
              << std::endl;

    pika::util::print_cdash_timing(
        "AsyncSpeedup", sequential_time_per_task / hierarchical_time_per_task);

    return pika::finalize();
}

int main(int argc, char* argv[])
{
    using namespace pika::program_options;
    options_description desc_commandline("Usage: " PIKA_APPLICATION_STRING " [options]");

    // clang-format off
    desc_commandline.add_options()
        ("tasks,t", value<std::size_t>(),
         "number of tasks to spawn sequentially (default: 128)")
        ("sub-tasks,s", value<std::size_t>(&num_level_tasks)->default_value(16),
         "number of tasks spawned per sub-spawn (default: 16)")
        ("spread,p", value<std::size_t>(&spread)->default_value(2),
         "number of sub-spawns per level (default: 2)")
        ("delay,d", value<std::uint64_t>(&delay_ns)->default_value(0),
        "time spent in the delay loop [ns]");
    // clang-format on

    // Initialize and run pika
    pika::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return pika::init(pika_main, argc, argv, init_args);
}
