//  Copyright (c) 2017-2018 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/init.hpp>
#include <pika/parallel/container_algorithms/merge.hpp>
#include <pika/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
int seed = std::random_device{}();
std::mt19937 rng(seed);

////////////////////////////////////////////////////////////////////////////
struct user_defined_type
{
    user_defined_type() = default;
    user_defined_type(int rand_no)
      : val(rand_no)
      , name(name_list[rng() % name_list.size()])
    {
    }

    bool operator<(user_defined_type const& t) const
    {
        if (this->name < t.name)
            return true;
        else if (this->name > t.name)
            return false;
        else
            return this->val < t.val;
    }

    bool operator>(user_defined_type const& t) const
    {
        if (this->name > t.name)
            return true;
        else if (this->name < t.name)
            return false;
        else
            return this->val > t.val;
    }

    bool operator==(user_defined_type const& t) const
    {
        return this->name == t.name && this->val == t.val;
    }

    user_defined_type operator+(int val) const
    {
        user_defined_type t(*this);
        t.val += val;
        return t;
    }

    static const std::vector<std::string> name_list;

    int val;
    std::string name;
};

const std::vector<std::string> user_defined_type::name_list{
    "ABB", "ABC", "ACB", "BASE", "CAA", "CAAA", "CAAB"};

struct random_fill
{
    random_fill(int rand_base, int range)
      : gen(rng())
      , dist(rand_base - range / 2, rand_base + range / 2)
    {
    }

    int operator()()
    {
        return dist(gen);
    }

    std::mt19937 gen;
    std::uniform_int_distribution<> dist;
};

////////////////////////////////////////////////////////////////////////////
template <typename DataType>
void test_merge(DataType)
{
    std::size_t const size1 = 300007, size2 = 123456;
    std::vector<DataType> src1(size1), src2(size2), dest_res(size1 + size2),
        dest_sol(size1 + size2);

    std::generate(std::begin(src1), std::end(src1), random_fill(0, 6));
    std::generate(std::begin(src2), std::end(src2), random_fill(0, 8));
    std::sort(std::begin(src1), std::end(src1));
    std::sort(std::begin(src2), std::end(src2));

    auto result = pika::ranges::merge(src1, src2, std::begin(dest_res));
    auto solution = std::merge(std::begin(src1), std::end(src1),
        std::begin(src2), std::end(src2), std::begin(dest_sol));

    PIKA_TEST(result.in1 == std::end(src1));
    PIKA_TEST(result.in2 == std::end(src2));

    bool equality = test::equal(
        std::begin(dest_res), result.out, std::begin(dest_sol), solution);

    PIKA_TEST(equality);
}

template <typename ExPolicy, typename DataType>
void test_merge(ExPolicy&& policy, DataType)
{
    static_assert(pika::is_execution_policy<ExPolicy>::value,
        "pika::is_execution_policy<ExPolicy>::value");

    std::size_t const size1 = 300007, size2 = 123456;
    std::vector<DataType> src1(size1), src2(size2), dest_res(size1 + size2),
        dest_sol(size1 + size2);

    std::generate(std::begin(src1), std::end(src1), random_fill(0, 6));
    std::generate(std::begin(src2), std::end(src2), random_fill(0, 8));
    std::sort(std::begin(src1), std::end(src1));
    std::sort(std::begin(src2), std::end(src2));

    auto result = pika::ranges::merge(policy, src1, src2, std::begin(dest_res));
    auto solution = std::merge(std::begin(src1), std::end(src1),
        std::begin(src2), std::end(src2), std::begin(dest_sol));

    PIKA_TEST(result.in1 == std::end(src1));
    PIKA_TEST(result.in2 == std::end(src2));

    bool equality = test::equal(
        std::begin(dest_res), result.out, std::begin(dest_sol), solution);

    PIKA_TEST(equality);
}

template <typename ExPolicy, typename DataType>
void test_merge_async(ExPolicy&& policy, DataType)
{
    static_assert(pika::is_execution_policy<ExPolicy>::value,
        "pika::is_execution_policy<ExPolicy>::value");

    std::size_t const size1 = 300007, size2 = 123456;
    std::vector<DataType> src1(size1), src2(size2), dest_res(size1 + size2),
        dest_sol(size1 + size2);

    std::generate(std::begin(src1), std::end(src1), random_fill(0, 6));
    std::generate(std::begin(src2), std::end(src2), random_fill(0, 8));
    std::sort(std::begin(src1), std::end(src1));
    std::sort(std::begin(src2), std::end(src2));

    auto f = pika::ranges::merge(policy, src1, src2, std::begin(dest_res));
    auto result = f.get();
    auto solution = std::merge(std::begin(src1), std::end(src1),
        std::begin(src2), std::end(src2), std::begin(dest_sol));

    PIKA_TEST(result.in1 == std::end(src1));
    PIKA_TEST(result.in2 == std::end(src2));

    bool equality = test::equal(
        std::begin(dest_res), result.out, std::begin(dest_sol), solution);

    PIKA_TEST(equality);
}

///////////////////////////////////////////////////////////////////////////////
template <typename DataType>
void test_merge()
{
    using namespace pika::execution;

    test_merge(DataType());

    test_merge(seq, DataType());
    test_merge(par, DataType());
    test_merge(par_unseq, DataType());

    test_merge_async(seq(task), DataType());
    test_merge_async(par(task), DataType());
}

void test_merge()
{
    test_merge<int>();
    test_merge<user_defined_type>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag, typename DataType>
void test_merge_etc(IteratorTag, DataType, int rand_base)
{
    using base_iterator = typename std::vector<DataType>::iterator;

    std::size_t const size1 = 300007, size2 = 123456;
    std::vector<DataType> src1(size1), src2(size2), dest_res(size1 + size2),
        dest_sol(size1 + size2);

    std::generate(std::begin(src1), std::end(src1), random_fill(rand_base, 6));
    std::generate(std::begin(src2), std::end(src2), random_fill(rand_base, 8));
    std::sort(std::begin(src1), std::end(src1));
    std::sort(std::begin(src2), std::end(src2));

    // Test projection.
    {
        using iterator = test::test_iterator<base_iterator, IteratorTag>;

        DataType val;
        pika::ranges::merge(
            iterator(std::begin(src1)), iterator(std::end(src1)),
            iterator(std::begin(src2)), iterator(std::end(src2)),
            iterator(std::begin(dest_res)),
            [](DataType const& a, DataType const& b) -> bool { return a < b; },
            [&val](DataType const&) -> DataType {
                // This is projection.
                return val;
            },
            [&val](DataType const&) -> DataType {
                // This is projection.
                return val + 1;
            });

        bool equality1 =
            std::equal(std::begin(src1), std::end(src1), std::begin(dest_res));
        bool equality2 = std::equal(
            std::begin(src2), std::end(src2), std::begin(dest_res) + size1);

        PIKA_TEST(equality1);
        PIKA_TEST(equality2);
    }
}

template <typename ExPolicy, typename IteratorTag, typename DataType>
void test_merge_etc(ExPolicy&& policy, IteratorTag, DataType, int rand_base)
{
    static_assert(pika::is_execution_policy<ExPolicy>::value,
        "pika::is_execution_policy<ExPolicy>::value");

    using base_iterator = typename std::vector<DataType>::iterator;

    std::size_t const size1 = 300007, size2 = 123456;
    std::vector<DataType> src1(size1), src2(size2), dest_res(size1 + size2),
        dest_sol(size1 + size2);

    std::generate(std::begin(src1), std::end(src1), random_fill(rand_base, 6));
    std::generate(std::begin(src2), std::end(src2), random_fill(rand_base, 8));
    std::sort(std::begin(src1), std::end(src1));
    std::sort(std::begin(src2), std::end(src2));

    // Test projection.
    {
        using iterator = test::test_iterator<base_iterator, IteratorTag>;

        DataType val;
        pika::ranges::merge(
            policy, iterator(std::begin(src1)), iterator(std::end(src1)),
            iterator(std::begin(src2)), iterator(std::end(src2)),
            iterator(std::begin(dest_res)),
            [](DataType const& a, DataType const& b) -> bool { return a < b; },
            [&val](DataType const&) -> DataType {
                // This is projection.
                return val;
            },
            [&val](DataType const&) -> DataType {
                // This is projection.
                return val + 1;
            });

        bool equality1 =
            std::equal(std::begin(src1), std::end(src1), std::begin(dest_res));
        bool equality2 = std::equal(
            std::begin(src2), std::end(src2), std::begin(dest_res) + size1);

        PIKA_TEST(equality1);
        PIKA_TEST(equality2);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag, typename DataType>
void test_merge_stable(IteratorTag, DataType, int rand_base)
{
    using ElemType = typename std::pair<DataType, int>;
    using base_iterator = typename std::vector<ElemType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size1 = 300007, size2 = 123456;
    std::vector<ElemType> src1(size1), src2(size2), dest(size1 + size2);

    int no = 0;
    auto rf = random_fill(rand_base, 6);
    std::generate(
        std::begin(src1), std::end(src1), [&no, &rf]() -> std::pair<int, int> {
            return {rf(), no++};
        });
    rf = random_fill(rand_base, 8);
    std::generate(
        std::begin(src2), std::end(src2), [&no, &rf]() -> std::pair<int, int> {
            return {rf(), no++};
        });
    std::sort(std::begin(src1), std::end(src1));
    std::sort(std::begin(src2), std::end(src2));

    pika::ranges::merge(
        iterator(std::begin(src1)), iterator(std::end(src1)),
        iterator(std::begin(src2)), iterator(std::end(src2)),
        iterator(std::begin(dest)),
        [](DataType const& a, DataType const& b) -> bool { return a < b; },
        [](ElemType const& elem) -> DataType const& {
            // This is projection.
            return elem.first;
        },
        [](ElemType const& elem) -> DataType const& {
            // This is projection.
            return elem.first;
        });

    bool stable = true;
    int check_count = 0;
    for (auto i = 1u; i < size1 + size2; ++i)
    {
        if (dest[i - 1].first == dest[i].first)
        {
            ++check_count;
            if (dest[i - 1].second > dest[i].second)
                stable = false;
        }
    }

    bool test_is_meaningful = check_count >= 100;

    PIKA_TEST(test_is_meaningful);
    PIKA_TEST(stable);
}

template <typename ExPolicy, typename IteratorTag, typename DataType>
void test_merge_stable(ExPolicy&& policy, IteratorTag, DataType, int rand_base)
{
    static_assert(pika::is_execution_policy<ExPolicy>::value,
        "pika::is_execution_policy<ExPolicy>::value");

    using ElemType = typename std::pair<DataType, int>;
    using base_iterator = typename std::vector<ElemType>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::size_t const size1 = 300007, size2 = 123456;
    std::vector<ElemType> src1(size1), src2(size2), dest(size1 + size2);

    int no = 0;
    auto rf = random_fill(rand_base, 6);
    std::generate(
        std::begin(src1), std::end(src1), [&no, &rf]() -> std::pair<int, int> {
            return {rf(), no++};
        });
    rf = random_fill(rand_base, 8);
    std::generate(
        std::begin(src2), std::end(src2), [&no, &rf]() -> std::pair<int, int> {
            return {rf(), no++};
        });
    std::sort(std::begin(src1), std::end(src1));
    std::sort(std::begin(src2), std::end(src2));

    pika::ranges::merge(
        policy, iterator(std::begin(src1)), iterator(std::end(src1)),
        iterator(std::begin(src2)), iterator(std::end(src2)),
        iterator(std::begin(dest)),
        [](DataType const& a, DataType const& b) -> bool { return a < b; },
        [](ElemType const& elem) -> DataType const& {
            // This is projection.
            return elem.first;
        },
        [](ElemType const& elem) -> DataType const& {
            // This is projection.
            return elem.first;
        });

    bool stable = true;
    int check_count = 0;
    for (auto i = 1u; i < size1 + size2; ++i)
    {
        if (dest[i - 1].first == dest[i].first)
        {
            ++check_count;
            if (dest[i - 1].second > dest[i].second)
                stable = false;
        }
    }

    bool test_is_meaningful = check_count >= 100;

    PIKA_TEST(test_is_meaningful);
    PIKA_TEST(stable);
}

template <typename IteratorTag>
void test_merge_stable()
{
    ////////// Test cases for checking whether the algorithm is stable.
    using namespace pika::execution;

    int rand_base = rng();

    test_merge_stable(IteratorTag(), int(), rand_base);
    test_merge_stable(seq, IteratorTag(), int(), rand_base);
    test_merge_stable(par, IteratorTag(), int(), rand_base);
    test_merge_stable(par_unseq, IteratorTag(), int(), rand_base);
    test_merge_stable(seq, IteratorTag(), user_defined_type(), rand_base);
    test_merge_stable(par, IteratorTag(), user_defined_type(), rand_base);
    test_merge_stable(par_unseq, IteratorTag(), user_defined_type(), rand_base);
}

template <typename IteratorTag>
void test_merge_etc()
{
    ////////// Test cases for checking whether the algorithm is stable.
    using namespace pika::execution;

    int rand_base = rng();

    ////////// Another test cases for justifying the implementation.
    test_merge_etc(IteratorTag(), user_defined_type(), rand_base);
    test_merge_etc(seq, IteratorTag(), user_defined_type(), rand_base);
    test_merge_etc(par, IteratorTag(), user_defined_type(), rand_base);
    test_merge_etc(par_unseq, IteratorTag(), user_defined_type(), rand_base);
}

int pika_main(pika::program_options::variables_map& vm)
{
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
        rng.seed(seed);
    }
    std::cout << "using seed: " << seed << std::endl;

    test_merge();
    test_merge_stable<std::random_access_iterator_tag>();
    test_merge_etc<std::random_access_iterator_tag>();

    return pika::finalize();
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
    pika::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    PIKA_TEST_EQ_MSG(pika::init(pika_main, argc, argv, init_args), 0,
        "pika main exited with non-zero status");

    return pika::util::report_errors();
}
