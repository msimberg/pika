//  Copyright (c) 2014-2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/local/init.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "any_of_tests.hpp"

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_any_of()
{
    struct proj
    {
        //This projection should cause tests to fail if it is not applied
        //because it causes predicate to evaluate the opposite
        constexpr std::size_t operator()(std::size_t x) const
        {
            return !static_cast<bool>(x);
        }
    };
    using namespace pika::execution;

    test_any_of(IteratorTag());
    test_any_of_ranges_seq(IteratorTag(), proj());

    test_any_of(seq, IteratorTag());
    test_any_of(par, IteratorTag());
    test_any_of(par_unseq, IteratorTag());

    test_any_of_ranges(seq, IteratorTag(), proj());
    test_any_of_ranges(par, IteratorTag(), proj());
    test_any_of_ranges(par_unseq, IteratorTag(), proj());

    test_any_of_async(seq(task), IteratorTag());
    test_any_of_async(par(task), IteratorTag());

    test_any_of_ranges_async(seq(task), IteratorTag(), proj());
    test_any_of_ranges_async(par(task), IteratorTag(), proj());
}

// template <typename IteratorTag>
// void test_any_of_exec()
// {
//     using namespace pika::execution;
//
//     {
//         pika::threads::executors::local_priority_queue_executor exec;
//         test_any_of(par(exec), IteratorTag());
//     }
//     {
//         pika::threads::executors::local_priority_queue_executor exec;
//         test_any_of(task(exec), IteratorTag());
//     }
//
//     {
//         pika::threads::executors::local_priority_queue_executor exec;
//         test_any_of(execution_policy(par(exec)), IteratorTag());
//     }
//     {
//         pika::threads::executors::local_priority_queue_executor exec;
//         test_any_of(execution_policy(task(exec)), IteratorTag());
//     }
// }

////////////////////////////////////////////////////////////////////////////
void any_of_test()
{
    test_any_of<std::random_access_iterator_tag>();
    test_any_of<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_any_of_exception()
{
    using namespace pika::execution;

    test_any_of_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_any_of_exception(seq, IteratorTag());
    test_any_of_exception(par, IteratorTag());

    test_any_of_exception_async(seq(task), IteratorTag());
    test_any_of_exception_async(par(task), IteratorTag());
}

////////////////////////////////////////////////////////////////////////////
void any_of_exception_test()
{
    test_any_of_exception<std::random_access_iterator_tag>();
    test_any_of_exception<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_any_of_bad_alloc()
{
    using namespace pika::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_any_of_bad_alloc(seq, IteratorTag());
    test_any_of_bad_alloc(par, IteratorTag());

    test_any_of_bad_alloc_async(seq(task), IteratorTag());
    test_any_of_bad_alloc_async(par(task), IteratorTag());
}

void any_of_bad_alloc_test()
{
    test_any_of_bad_alloc<std::random_access_iterator_tag>();
    test_any_of_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    any_of_test();
    any_of_exception_test();
    any_of_bad_alloc_test();
    return pika::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace pika::program_options;
    options_description desc_commandline(
        "Usage: " PIKA_APPLICATION_STRING " [options]");

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
