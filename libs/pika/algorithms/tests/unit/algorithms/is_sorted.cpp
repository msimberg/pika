//  Copyright (c) 2015 Daniel Bourgeois
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/local/init.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "is_sorted_tests.hpp"

////////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_sorted1()
{
    using namespace pika::execution;
    test_sorted1(seq, IteratorTag());
    test_sorted1(par, IteratorTag());
    test_sorted1(par_unseq, IteratorTag());

    test_sorted1_async(seq(task), IteratorTag());
    test_sorted1_async(par(task), IteratorTag());

    test_sorted1_seq(IteratorTag());
}

void sorted_test1()
{
    test_sorted1<std::random_access_iterator_tag>();
    test_sorted1<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_sorted2()
{
    using namespace pika::execution;
    test_sorted2(seq, IteratorTag());
    test_sorted2(par, IteratorTag());
    test_sorted2(par_unseq, IteratorTag());

    test_sorted2_async(seq(task), IteratorTag());
    test_sorted2_async(par(task), IteratorTag());

    test_sorted2_seq(IteratorTag());
}

void sorted_test2()
{
    test_sorted2<std::random_access_iterator_tag>();
    test_sorted2<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_sorted3()
{
    using namespace pika::execution;
    test_sorted3(seq, IteratorTag());
    test_sorted3(par, IteratorTag());
    test_sorted3(par_unseq, IteratorTag());

    test_sorted3_async(seq(task), IteratorTag());
    test_sorted3_async(par(task), IteratorTag());

    test_sorted3_seq(IteratorTag());
}

void sorted_test3()
{
    test_sorted3<std::random_access_iterator_tag>();
    test_sorted3<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_sorted_exception()
{
    using namespace pika::execution;
    //If the execution policy object is of type vector_execution_policy,
    //  std::terminate shall be called. Therefore we do not test exceptions
    //  with a vector execution policy
    test_sorted_exception(seq, IteratorTag());
    test_sorted_exception(par, IteratorTag());

    test_sorted_exception_async(seq(task), IteratorTag());
    test_sorted_exception_async(par(task), IteratorTag());

    test_sorted_exception_seq(IteratorTag());
}

void sorted_exception_test()
{
    test_sorted_exception<std::random_access_iterator_tag>();
    test_sorted_exception<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_sorted_bad_alloc()
{
    using namespace pika::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_sorted_bad_alloc(par, IteratorTag());
    test_sorted_bad_alloc(seq, IteratorTag());

    test_sorted_bad_alloc_async(seq(task), IteratorTag());
    test_sorted_bad_alloc_async(par(task), IteratorTag());

    test_sorted_bad_alloc_seq(IteratorTag());
}

void sorted_bad_alloc_test()
{
    test_sorted_bad_alloc<std::random_access_iterator_tag>();
    test_sorted_bad_alloc<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    sorted_test1();
    sorted_test2();
    sorted_test3();
    sorted_exception_test();
    sorted_bad_alloc_test();
    return pika::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace pika::program_options;
    options_description desc_commandline(
        "Usage: " PIKA_APPLICATION_STRING " [options]");

    std::vector<std::string> const cfg = {"pika.os_threads=all"};

    pika::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    PIKA_TEST_EQ_MSG(pika::local::init(pika_main, argc, argv, init_args), 0,
        "pika main exited with non-zero status");

    return pika::util::report_errors();
}
