//  Copyright (c) 2017 Bruno Pitrus
//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/container_algorithms/all_any_none.hpp

#pragma once

#if defined(DOXYGEN)

namespace pika { namespace ranges {
    // clang-format off

    ///  Checks if unary predicate \a f returns true for no elements in the
    ///  range \a rng.
    ///
    /// \note   Complexity: At most std::distance(begin(rng), end(rng))
    ///         applications of the predicate \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a none_of requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a parallel::detail::projection_identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed
    ///                     to it. The type \a Type must be such that an object
    ///                     of type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a none_of algorithm returns a \a pika::future<bool> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a bool
    ///           otherwise.
    ///           The \a none_of algorithm returns true if the unary predicate
    ///           \a f returns true for no elements in the range, false
    ///           otherwise. It returns true if the range is empty.
    ///
    template <typename ExPolicy, typename Rng, typename F,
        typename Proj = parallel::detail::projection_identity>
    typename pika::parallel::detail::algorithm_result<ExPolicy, bool>::type
    none_of(ExPolicy&& policy, Rng&& rng, F&& f, Proj&& proj = Proj());

    /// Checks if unary predicate \a f returns true for at least one element
    /// in the range \a rng.
    ///
    /// \note   Complexity: At most std::distance(begin(rng), end(rng))
    ///         applications of the predicate \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a none_of requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a parallel::detail::projection_identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed
    ///                     to it. The type \a Type must be such that an object
    ///                     of type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a any_of algorithm returns a \a pika::future<bool> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a bool otherwise.
    ///           The \a any_of algorithm returns true if the unary predicate
    ///           \a f returns true for at least one element in the range,
    ///           false otherwise. It returns false if the range is empty.
    ///
    template <typename ExPolicy, typename Rng, typename F,
        typename Proj = parallel::detail::projection_identity>
    typename pika::parallel::detail::algorithm_result<ExPolicy, bool>::type
    any_of(ExPolicy&& policy, Rng&& rng, F&& f, Proj&& proj = Proj());

    /// Checks if unary predicate \a f returns true for all elements in the
    /// range \a rng.
    ///
    /// \note   Complexity: At most std::distance(begin(rng), end(rng))
    ///         applications of the predicate \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam Rng         The type of the source range used (deduced).
    ///                     The iterators extracted from this range type must
    ///                     meet the requirements of an input iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a none_of requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a parallel::detail::projection_identity
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param rng          Refers to the sequence of elements the algorithm
    ///                     will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed
    ///                     to it. The type \a Type must be such that an object
    ///                     of type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements as a
    ///                     projection operation before the actual predicate
    ///                     \a is invoked.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a all_of algorithm returns a \a pika::future<bool> if
    ///           the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a bool otherwise.
    ///           The \a all_of algorithm returns true if the unary predicate
    ///           \a f returns true for all elements in the range, false
    ///           otherwise. It returns true if the range is empty.
    ///
    template <typename ExPolicy, typename Rng, typename F,
        typename Proj = parallel::detail::projection_identity>
    typename pika::parallel::detail::algorithm_result<ExPolicy, bool>::type
    all_of(ExPolicy&& policy, Rng&& rng, F&& f, Proj&& proj = Proj());

    // clang-format on
}}    // namespace pika::ranges

#else    // DOXYGEN

#include <pika/config.hpp>
#include <pika/concepts/concepts.hpp>
#include <pika/iterator_support/range.hpp>
#include <pika/iterator_support/traits/is_range.hpp>
#include <pika/parallel/util/detail/sender_util.hpp>

#include <pika/algorithms/traits/projected_range.hpp>
#include <pika/executors/execution_policy.hpp>
#include <pika/parallel/algorithms/all_any_none.hpp>
#include <pika/parallel/util/projection_identity.hpp>

#include <type_traits>
#include <utility>

namespace pika::ranges {
    ///////////////////////////////////////////////////////////////////////////
    // CPO for pika::ranges::none_of
    inline constexpr struct none_of_t final
      : pika::detail::tag_parallel_algorithm<none_of_t>
    {
    private:
        // clang-format off
        template <typename ExPolicy, typename Rng, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::is_execution_policy<ExPolicy>::value &&
                pika::traits::is_range<Rng>::value &&
                pika::parallel::detail::is_projected_range<Proj, Rng>::value &&
                pika::parallel::detail::is_indirect_callable<ExPolicy, F,
                    pika::parallel::detail::projected_range<Proj, Rng>
                >::value
            )>
        // clang-format on
        friend typename pika::parallel::detail::algorithm_result<ExPolicy,
            bool>::type
        tag_fallback_invoke(none_of_t, ExPolicy&& policy, Rng&& rng, F&& f,
            Proj&& proj = Proj())
        {
            using iterator_type =
                typename pika::traits::range_iterator<Rng>::type;

            static_assert(
                pika::traits::is_forward_iterator<iterator_type>::value,
                "Required at least forward iterator.");

            return pika::parallel::detail::none_of().call(
                PIKA_FORWARD(ExPolicy, policy), pika::util::begin(rng),
                pika::util::end(rng), PIKA_FORWARD(F, f),
                PIKA_FORWARD(Proj, proj));
        }

        // clang-format off
        template <typename ExPolicy, typename Iter, typename Sent, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::is_execution_policy<ExPolicy>::value &&
                pika::traits::is_iterator<Iter>::value &&
                pika::traits::is_sentinel_for<Sent, Iter>::value &&
                pika::parallel::detail::is_projected<Proj, Iter>::value &&
                pika::parallel::detail::is_indirect_callable<ExPolicy, F,
                    pika::parallel::detail::projected<Proj, Iter>
                >::value
            )>
        // clang-format on
        friend typename pika::parallel::detail::algorithm_result<ExPolicy,
            bool>::type
        tag_fallback_invoke(none_of_t, ExPolicy&& policy, Iter first, Sent last,
            F&& f, Proj&& proj = Proj())
        {
            static_assert(pika::traits::is_forward_iterator<Iter>::value,
                "Required at least forward iterator.");

            return pika::parallel::detail::none_of().call(
                PIKA_FORWARD(ExPolicy, policy), first, last, PIKA_FORWARD(F, f),
                PIKA_FORWARD(Proj, proj));
        }

        // clang-format off
        template <typename Rng, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::traits::is_range<Rng>::value &&
                pika::parallel::detail::is_projected_range<Proj, Rng>::value &&
                pika::parallel::detail::is_indirect_callable<
                    pika::execution::sequenced_policy, F,
                    pika::parallel::detail::projected_range<Proj, Rng>
                >::value
            )>
        // clang-format on
        friend bool tag_fallback_invoke(
            none_of_t, Rng&& rng, F&& f, Proj&& proj = Proj())
        {
            using iterator_type =
                typename pika::traits::range_iterator<Rng>::type;

            static_assert(pika::traits::is_input_iterator<iterator_type>::value,
                "Required at least input iterator.");

            return pika::parallel::detail::none_of().call(pika::execution::seq,
                pika::util::begin(rng), pika::util::end(rng),
                PIKA_FORWARD(F, f), PIKA_FORWARD(Proj, proj));
        }

        // clang-format off
        template <typename Iter, typename Sent, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::traits::is_iterator<Iter>::value &&
                pika::traits::is_sentinel_for<Sent, Iter>::value &&
                pika::parallel::detail::is_projected<Proj, Iter>::value &&
                pika::parallel::detail::is_indirect_callable<
                    pika::execution::sequenced_policy, F,
                    pika::parallel::detail::projected<Proj, Iter>
                >::value
            )>
        // clang-format on
        friend bool tag_fallback_invoke(
            none_of_t, Iter first, Sent last, F&& f, Proj&& proj = Proj())
        {
            static_assert(pika::traits::is_input_iterator<Iter>::value,
                "Required at least input iterator.");

            return pika::parallel::detail::none_of().call(pika::execution::seq,
                first, last, PIKA_FORWARD(F, f), PIKA_FORWARD(Proj, proj));
        }
    } none_of{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for pika::ranges::any_of
    inline constexpr struct any_of_t final
      : pika::detail::tag_parallel_algorithm<any_of_t>
    {
    private:
        // clang-format off
        template <typename ExPolicy, typename Rng, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::is_execution_policy<ExPolicy>::value &&
                pika::traits::is_range<Rng>::value &&
                pika::parallel::detail::is_projected_range<Proj, Rng>::value &&
                pika::parallel::detail::is_indirect_callable<ExPolicy, F,
                    pika::parallel::detail::projected_range<Proj, Rng>
                >::value
            )>
        // clang-format on
        friend typename pika::parallel::detail::algorithm_result<ExPolicy,
            bool>::type
        tag_fallback_invoke(
            any_of_t, ExPolicy&& policy, Rng&& rng, F&& f, Proj&& proj = Proj())
        {
            using iterator_type =
                typename pika::traits::range_iterator<Rng>::type;

            static_assert(
                pika::traits::is_forward_iterator<iterator_type>::value,
                "Required at least forward iterator.");

            return pika::parallel::detail::any_of().call(
                PIKA_FORWARD(ExPolicy, policy), pika::util::begin(rng),
                pika::util::end(rng), PIKA_FORWARD(F, f),
                PIKA_FORWARD(Proj, proj));
        }

        // clang-format off
        template <typename ExPolicy, typename Iter, typename Sent, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::is_execution_policy<ExPolicy>::value &&
                pika::traits::is_iterator<Iter>::value &&
                pika::traits::is_sentinel_for<Sent, Iter>::value &&
                pika::parallel::detail::is_projected<Proj, Iter>::value &&
                pika::parallel::detail::is_indirect_callable<ExPolicy, F,
                    pika::parallel::detail::projected<Proj, Iter>
                >::value
            )>
        // clang-format on
        friend typename pika::parallel::detail::algorithm_result<ExPolicy,
            bool>::type
        tag_fallback_invoke(any_of_t, ExPolicy&& policy, Iter first, Sent last,
            F&& f, Proj&& proj = Proj())
        {
            static_assert(pika::traits::is_forward_iterator<Iter>::value,
                "Required at least forward iterator.");

            return pika::parallel::detail::any_of().call(
                PIKA_FORWARD(ExPolicy, policy), first, last, PIKA_FORWARD(F, f),
                PIKA_FORWARD(Proj, proj));
        }

        // clang-format off
        template <typename Rng, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::traits::is_range<Rng>::value&&
                pika::parallel::detail::is_projected_range<Proj, Rng>::value&&
                pika::parallel::detail::is_indirect_callable<
                pika::execution::sequenced_policy, F,
                    pika::parallel::detail::projected_range<Proj, Rng>
                >::value
            )>
        // clang-format on
        friend bool tag_fallback_invoke(
            any_of_t, Rng&& rng, F&& f, Proj&& proj = Proj())
        {
            using iterator_type =
                typename pika::traits::range_iterator<Rng>::type;

            static_assert(pika::traits::is_input_iterator<iterator_type>::value,
                "Required at least input iterator.");

            return pika::parallel::detail::any_of().call(pika::execution::seq,
                pika::util::begin(rng), pika::util::end(rng),
                PIKA_FORWARD(F, f), PIKA_FORWARD(Proj, proj));
        }

        // clang-format off
        template <typename Iter, typename Sent, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::traits::is_iterator<Iter>::value &&
                pika::traits::is_sentinel_for<Sent, Iter>::value &&
                pika::parallel::detail::is_projected<Proj, Iter>::value &&
                pika::parallel::detail::is_indirect_callable<
                pika::execution::sequenced_policy, F,
                    pika::parallel::detail::projected<Proj, Iter>
                >::value
            )>
        // clang-format on
        friend bool tag_fallback_invoke(
            any_of_t, Iter first, Sent last, F&& f, Proj&& proj = Proj())
        {
            static_assert(pika::traits::is_input_iterator<Iter>::value,
                "Required at least input iterator.");

            return pika::parallel::detail::any_of().call(pika::execution::seq,
                first, last, PIKA_FORWARD(F, f), PIKA_FORWARD(Proj, proj));
        }
    } any_of{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for pika::ranges::all_of
    inline constexpr struct all_of_t final
      : pika::detail::tag_parallel_algorithm<all_of_t>
    {
    private:
        // clang-format off
        template <typename ExPolicy, typename Rng, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::is_execution_policy<ExPolicy>::value &&
                pika::traits::is_range<Rng>::value &&
                pika::parallel::detail::is_projected_range<Proj, Rng>::value &&
                pika::parallel::detail::is_indirect_callable<ExPolicy, F,
                    pika::parallel::detail::projected_range<Proj, Rng>
                >::value
            )>
        // clang-format on
        friend typename pika::parallel::detail::algorithm_result<ExPolicy,
            bool>::type
        tag_fallback_invoke(
            all_of_t, ExPolicy&& policy, Rng&& rng, F&& f, Proj&& proj = Proj())
        {
            using iterator_type =
                typename pika::traits::range_iterator<Rng>::type;

            static_assert(
                pika::traits::is_forward_iterator<iterator_type>::value,
                "Required at least forward iterator.");

            return pika::parallel::detail::all_of().call(
                PIKA_FORWARD(ExPolicy, policy), pika::util::begin(rng),
                pika::util::end(rng), PIKA_FORWARD(F, f),
                PIKA_FORWARD(Proj, proj));
        }

        // clang-format off
        template <typename ExPolicy, typename Iter, typename Sent, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::is_execution_policy<ExPolicy>::value &&
                pika::traits::is_iterator<Iter>::value &&
                pika::traits::is_sentinel_for<Sent, Iter>::value &&
                pika::parallel::detail::is_projected<Proj, Iter>::value &&
                pika::parallel::detail::is_indirect_callable<ExPolicy, F,
                    pika::parallel::detail::projected<Proj, Iter>
                >::value
            )>
        // clang-format on
        friend typename pika::parallel::detail::algorithm_result<ExPolicy,
            bool>::type
        tag_fallback_invoke(all_of_t, ExPolicy&& policy, Iter first, Sent last,
            F&& f, Proj&& proj = Proj())
        {
            static_assert(pika::traits::is_forward_iterator<Iter>::value,
                "Required at least forward iterator.");

            return pika::parallel::detail::all_of().call(
                PIKA_FORWARD(ExPolicy, policy), first, last, PIKA_FORWARD(F, f),
                PIKA_FORWARD(Proj, proj));
        }

        // clang-format off
        template <typename Rng, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::traits::is_range<Rng>::value &&
                pika::parallel::detail::is_projected_range<Proj, Rng>::value &&
                pika::parallel::detail::is_indirect_callable<
                pika::execution::sequenced_policy, F,
                    pika::parallel::detail::projected_range<Proj, Rng>
                >::value
            )>
        // clang-format on
        friend bool tag_fallback_invoke(
            all_of_t, Rng&& rng, F&& f, Proj&& proj = Proj())
        {
            using iterator_type =
                typename pika::traits::range_iterator<Rng>::type;

            static_assert(pika::traits::is_input_iterator<iterator_type>::value,
                "Required at least input iterator.");

            return pika::parallel::detail::all_of().call(pika::execution::seq,
                pika::util::begin(rng), pika::util::end(rng),
                PIKA_FORWARD(F, f), PIKA_FORWARD(Proj, proj));
        }

        // clang-format off
        template <typename Iter, typename Sent, typename F,
            typename Proj = pika::parallel::detail::projection_identity,
            PIKA_CONCEPT_REQUIRES_(
                pika::traits::is_iterator<Iter>::value &&
                pika::traits::is_sentinel_for<Sent, Iter>::value &&
                pika::parallel::detail::is_projected<Proj, Iter>::value &&
                pika::parallel::detail::is_indirect_callable<
                pika::execution::sequenced_policy, F,
                    pika::parallel::detail::projected<Proj, Iter>
                >::value
            )>
        // clang-format on
        friend bool tag_fallback_invoke(
            all_of_t, Iter first, Sent last, F&& f, Proj&& proj = Proj())
        {
            static_assert(pika::traits::is_input_iterator<Iter>::value,
                "Required at least input iterator.");

            return pika::parallel::detail::all_of().call(pika::execution::seq,
                first, last, PIKA_FORWARD(F, f), PIKA_FORWARD(Proj, proj));
        }
    } all_of{};

}    // namespace pika::ranges

#endif    // DOXYGEN
