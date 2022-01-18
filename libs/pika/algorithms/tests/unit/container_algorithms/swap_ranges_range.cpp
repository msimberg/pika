//  Copyright (c) 2018 Christopher Ogle
//  Copyright (c) 2020 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/iterator_support/tests/iter_sent.hpp>
#include <pika/local/init.hpp>
#include <pika/modules/testing.hpp>
#include <pika/parallel/container_algorithms/swap_ranges.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
unsigned int seed;
std::mt19937 gen;
std::uniform_int_distribution<> dis(1, 10007);

template <typename IteratorTag>
void test_swap_ranges_sent(IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());

    std::size_t len = std::rand() % 10005 + 1;
    std::size_t begin_num = std::rand();

    std::iota(std::begin(c), std::begin(c) + len, begin_num);
    d = c;
    std::reverse(std::begin(d), std::begin(d) + len);

    std::fill(std::begin(c) + len, std::end(c), 100);
    std::fill(std::begin(d) + len, std::end(d), 200);

    pika::ranges::swap_ranges(std::begin(c), sentinel<std::size_t>{100},
        std::begin(d), sentinel<std::size_t>{200});

    std::size_t count = 0;
    std::for_each(
        std::begin(d), std::end(d), [&count, len, begin_num](std::size_t v) {
            if (count < len)
            {
                PIKA_TEST_EQ(v, count + begin_num);
                count++;
            }
            else
            {
                PIKA_TEST_EQ(v, static_cast<std::size_t>(200));
            }
        });

    std::for_each(
        std::begin(c), std::end(c), [&count, begin_num](std::size_t v) {
            if (count >= 1)
            {
                PIKA_TEST_EQ(v, count + begin_num - 1);
                count--;
            }
            else
            {
                PIKA_TEST_EQ(v, static_cast<std::size_t>(100));
            }
        });
}

template <typename ExPolicy, typename IteratorTag>
void test_swap_ranges_sent(ExPolicy policy, IteratorTag)
{
    static_assert(pika::is_execution_policy<ExPolicy>::value,
        "pika::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());

    std::size_t len = std::rand() % 10005 + 1;
    std::size_t begin_num = std::rand();

    std::iota(std::begin(c), std::begin(c) + len, begin_num);
    d = c;
    std::reverse(std::begin(d), std::begin(d) + len);

    std::fill(std::begin(c) + len, std::end(c), 100);
    std::fill(std::begin(d) + len, std::end(d), 200);

    pika::ranges::swap_ranges(policy, std::begin(c), sentinel<std::size_t>{100},
        std::begin(d), sentinel<std::size_t>{200});

    std::size_t count = 0;
    std::for_each(
        std::begin(d), std::end(d), [&count, len, begin_num](std::size_t v) {
            if (count < len)
            {
                PIKA_TEST_EQ(v, count + begin_num);
                count++;
            }
            else
            {
                PIKA_TEST_EQ(v, static_cast<std::size_t>(200));
            }
        });

    std::for_each(
        std::begin(c), std::end(c), [&count, begin_num](std::size_t v) {
            if (count >= 1)
            {
                PIKA_TEST_EQ(v, count + begin_num - 1);
                count--;
            }
            else
            {
                PIKA_TEST_EQ(v, static_cast<std::size_t>(100));
            }
        });
}

template <typename IteratorTag>
void test_swap_ranges(IteratorTag)
{
    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::vector<std::size_t> e(c.size());

    std::iota(std::begin(c), std::end(c), std::rand());
    d = c;
    e = c;
    std::reverse(std::begin(d), std::end(d));

    pika::ranges::swap_ranges(c, d);

    PIKA_TEST(std::equal(std::begin(c), std::end(c), e.rbegin()));
    PIKA_TEST(std::equal(std::begin(d), std::end(d), e.begin()));
}

template <typename ExPolicy, typename IteratorTag>
void test_swap_ranges(ExPolicy policy, IteratorTag)
{
    static_assert(pika::is_execution_policy<ExPolicy>::value,
        "pika::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::vector<std::size_t> e(c.size());

    std::iota(std::begin(c), std::end(c), std::rand());
    d = c;
    e = c;
    std::reverse(std::begin(d), std::end(d));

    pika::ranges::swap_ranges(policy, c, d);

    PIKA_TEST(std::equal(std::begin(c), std::end(c), e.rbegin()));
    PIKA_TEST(std::equal(std::begin(d), std::end(d), e.begin()));
}

template <typename ExPolicy, typename IteratorTag>
void test_swap_ranges_async(ExPolicy policy, IteratorTag)
{
    static_assert(pika::is_execution_policy<ExPolicy>::value,
        "pika::is_execution_policy<ExPolicy>::value");

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::vector<std::size_t> e(c.size());

    std::iota(std::begin(c), std::end(c), std::rand());
    d = c;
    e = c;
    std::reverse(std::begin(d), std::end(d));

    auto fut = pika::ranges::swap_ranges(policy, c, d);
    fut.wait();

    PIKA_TEST(std::equal(std::begin(c), std::end(c), e.rbegin()));
    PIKA_TEST(std::equal(std::begin(d), std::end(d), e.begin()));
}

template <typename IteratorTag>
void test_swap_ranges()
{
    using namespace pika::execution;

    test_swap_ranges(IteratorTag());
    test_swap_ranges(seq, IteratorTag());
    test_swap_ranges(par, IteratorTag());
    test_swap_ranges(par_unseq, IteratorTag());

    test_swap_ranges_async(seq(task), IteratorTag());
    test_swap_ranges_async(par(task), IteratorTag());

    test_swap_ranges_sent(IteratorTag());
    test_swap_ranges_sent(seq, IteratorTag());
    test_swap_ranges_sent(par, IteratorTag());
    test_swap_ranges_sent(par_unseq, IteratorTag());
}

void swap_ranges_test()
{
    test_swap_ranges<std::random_access_iterator_tag>();
    test_swap_ranges<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
int pika_main(pika::program_options::variables_map& vm)
{
    unsigned int seed1 = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed1 = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed1 << std::endl;
    std::srand(seed1);

    seed = seed1;
    gen = std::mt19937(seed);

    swap_ranges_test();
    return pika::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace pika::program_options;
    options_description desc_commandline(
        "Usage: " PIKA_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"pika.os_threads=all"};

    // Initialize and run pika
    pika::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    PIKA_TEST_EQ_MSG(pika::local::init(pika_main, argc, argv, init_args), 0,
        "pika main exited with non-zero status");

    return pika::util::report_errors();
}
