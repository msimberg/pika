// Copyright (c) 2023 ETH Zurich
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/concurrency/detail/arrival_tree.hpp>
#include <pika/execution.hpp>
#include <pika/init.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>

#include <random>

using pika::detail::arrival_tree;
namespace ex = pika::execution::experimental;
namespace tt = pika::this_thread::experimental;

unsigned int seed = static_cast<unsigned int>(std::time(nullptr));

void sequential(arrival_tree& t, std::size_t n)
{
    for (std::size_t i = 0; i < n - 1; ++i)
    {
        PIKA_TEST(!t.arrive(i));
    }

    PIKA_TEST(t.arrive(n - 1));
}

void sequential_reverse(arrival_tree& t, std::size_t n)
{
    for (std::size_t i = n - 1; i > 0; --i)
    {
        PIKA_TEST(!t.arrive(i));
    }

    PIKA_TEST(t.arrive(0));
}

void sequential_random(arrival_tree& t, std::size_t n)
{
    std::vector<std::size_t> is(n);
    std::iota(is.begin(), is.end(), 0);

    std::mt19937 g(seed + n);
    std::shuffle(is.begin(), is.end(), g);

    for (std::size_t i = 0; i < n - 1; ++i)
    {
        PIKA_TEST(!t.arrive(is.at(i)));
    }

    PIKA_TEST(t.arrive(is.at(n - 1)));
}

void tasks(arrival_tree& t, std::size_t n)
{
    std::atomic<std::size_t> arrivals{0};
    std::atomic<std::size_t> last_arrivals{0};

    std::vector<ex::unique_any_sender<>> v;
    v.reserve(n);

    for (std::size_t i = 0; i < n; ++i)
    {
        v.push_back(ex::schedule(ex::thread_pool_scheduler{}) |
            ex::then([&, i, n]() mutable {
                PIKA_TEST_LT(arrivals, n);
                PIKA_TEST_EQ(last_arrivals, std::size_t(0));
                ++arrivals;
                if (t.arrive(i))
                {
                    ++last_arrivals;
                    PIKA_TEST_EQ(arrivals, n);
                    PIKA_TEST_EQ(last_arrivals, std::size_t(1));
                }
            }));
    }

    tt::sync_wait(ex::when_all_vector(std::move(v)));

    PIKA_TEST_EQ(arrivals, n);
    PIKA_TEST_EQ(last_arrivals, std::size_t(1));
}

template <typename F>
void test(std::size_t n, F f)
{
    arrival_tree t{n};
    f(t, n);
}

int pika_main(pika::program_options::variables_map& vm)
{
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
    }

    std::cout << "using seed: " << seed << std::endl;

    for (std::size_t n = 1; n < 100; ++n)
    {
        test(n, &sequential);
        test(n, &sequential_reverse);
        test(n, &sequential_random);
        test(n, &tasks);
    }

    pika::finalize();

    return 0;
}

int main(int argc, char* argv[])
{
    using namespace pika::program_options;
    options_description desc_commandline(
        "Usage: " PIKA_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    pika::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return pika::init(pika_main, argc, argv, init_args);
}
