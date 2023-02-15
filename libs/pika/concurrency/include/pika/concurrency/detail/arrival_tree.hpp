//  Copyright (c) 2023 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/assert.hpp>

#include <atomic>
#include <cstdint>
#include <vector>

#include <stdexcept>

#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <iostream>

namespace pika::detail {
    class arrival_tree
    {
    public:
        explicit arrival_tree(std::size_t n)
          : n(n)
          , n_full(next_power_of_fan_out(n))
          , tree(n_full - 1)
#if defined(PIKA_DEBUG)
          , arrivals(n)
#endif
        {
            PIKA_ASSERT(n >= 1);

            // fmt::print(std::cerr, "arrival_tree ctor, n = {}, n_full = {}\n", n,
            //     n_full);

            // TODO: no for loops

            // Fill the tree
            for (std::size_t i = 0; i < n_full - 1; ++i)
            {
                tree.at(i) = fan_out;
                // fmt::print(std::cerr, "arrival_tree ctor, tree[{}] = {}\n", i,
                //     fan_out);
            }

            // Reduce counts for the missing nodes at the last level of the tree
            for (std::size_t i = n; i < n_full; ++i)
            {
                arrive(i);
            }

#if defined(PIKA_DEBUG)
            for (std::size_t i = 0; i < arrivals.size(); ++i)
            {
                arrivals.at(i) = 0;
            }
#endif
        }

        ~arrival_tree()
        {
#if defined(PIKA_DEBUG)
            for (std::size_t i = 0; i < n; ++i)
            {
                // fmt::print(std::cerr, "n = {}, i = {}, arrivals[i] = {}\n", n, i, arrivals[i]);
                PIKA_ASSERT(arrivals[i] == 1);
            }
#endif
        }

        arrival_tree(arrival_tree&&) = delete;
        arrival_tree(arrival_tree const&) = delete;
        arrival_tree& operator=(arrival_tree&&) = delete;
        arrival_tree& operator=(arrival_tree const&) = delete;

        std::size_t size() const noexcept
        {
            return n;
        }

        /// Arrives index i at the tree. If i is the last to arrive at the tree,
        /// returns true. Otherwise, returns false.
        bool arrive(std::size_t i) noexcept
        {
            // PIKA_ASSERT(i < n);
            if (i < n)
            {
                PIKA_ASSERT(++arrivals.at(i) == 1);
            }

            // std::cerr << "arrive, i = " << i << '\n';

            // TODO: No special case?
            if (n == 1)
            {
                return true;
            }

            // TODO
            std::size_t node = parent(n_full - 1 + i);

            do
            {
                // std::cerr << "arrive, node = " << node << '\n';

                // TODO: Memory order
                PIKA_ASSERT(tree.at(node) > 0);
                PIKA_ASSERT(tree.at(node) <= fan_out);
                std::size_t const new_value = --tree.at(node);
                PIKA_ASSERT(new_value < fan_out);

                // std::cerr << "arrive, node = " << node
                //           << ", new_value = " << new_value << '\n';

                // If the new value is 0 all children have arrived at the
                // current node of the tree. We continue further up the tree or
                // stop.
                if (new_value == 0)
                {
                    // If this was the root of the tree everyone else has
                    // arrived and we are the last to arrive. We can stop,
                    // returning true.
                    if (node == root)
                    {
                        // std::cerr << "arrive, node = " << node
                        //           << ", stopping with true\n";
                        return true;
                    }

                    // If this wasn't the root, we continue up the tree.
                    // std::cerr << "arrive, node = " << node
                    //           << ", moving up the tree\n";
                    node = parent(node);
                }
                // If the new value isn't yet 0 some other thread will move up
                // the tree on this node. We can stop, returning false. We are
                // not the last one to arrive.
                else
                {
                    // std::cerr
                    //     << "arrive, node = " << node
                    //     << ", stopping because not yet done at this node\n";
                    return false;
                }
            } while (true);

            PIKA_UNREACHABLE;
            return true;
        }

    private:
        static std::size_t parent(std::size_t i) noexcept
        {
            // TODO: i = 0 -> return 0?
            // TODO: PIKA_ASSERT(i < 2 * n);
            // TODO
            // std::cerr << "parent: i = " << i
            //           << ", parent = " << ((i - 1) / fan_out) << '\n';
            return (i - 1) / fan_out;
        }

        static std::size_t next_power_of_fan_out(std::size_t i) noexcept
        {
            // TODO: 1 -> return 1 or 2?
            std::size_t j = 1;

            while (i > j)
            {
                j *= fan_out;
            }

            return j;
        }

        static constexpr std::size_t fan_out = 2;
        static constexpr std::size_t root = 0;

        const std::size_t n;
        const std::size_t n_full;
        // TODO: cache line padding?
        std::vector<std::atomic<std::size_t>> tree;
#if defined(PIKA_DEBUG)
        std::vector<std::atomic<std::size_t>> arrivals;
#endif
    };
}    // namespace pika::detail
