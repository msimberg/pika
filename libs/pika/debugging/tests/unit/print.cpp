//  Copyright (c) 2020 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The purpose of this test is to ensure that when enable_print is false
// that function arguments and other parameters used in the print statements
// are completely elided

#include <pika/debugging/print.hpp>
#include <pika/modules/timing.hpp>
#include <pika/testing.hpp>
#include <pika/threading_base/print.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>

namespace pika {
    // this is an enabled debug object that should output messages
    inline constexpr pika::debug::detail::enable_print<true> p_enabled("PRINT  ");
    // this is disabled and we want it to have zero footprint
    inline constexpr pika::debug::detail::enable_print<false> p_disabled("PRINT  ");
}    // namespace pika

int increment(std::atomic<int>& counter) { return ++counter; }

int main()
{
    // some counters we will use for checking if anything happens or not
    std::atomic<int> enabled_counter(0);
    std::atomic<int> disabled_counter(0);

    using namespace pika;
    using namespace pika::debug::detail;

    // ---------------------------------------------------------
    // Test if normal debug messages trigger argument evaluation
    // use the PIKA_DETAIL_DP_LAZY macro to prevent evaluation when disabled
    // we expect the counter to increment
    p_enabled.debug("Increment", increment(enabled_counter));
    PIKA_TEST_EQ(enabled_counter.load(), 1);

    // we expect the counter to increment as LAZY will be evaluated
    p_enabled.debug("Increment", PIKA_DETAIL_DP_LAZY(p_enabled, increment(enabled_counter)));
    PIKA_TEST_EQ(enabled_counter.load(), 2);

    // we do not expect the counter to increment
    if (p_disabled.is_enabled()) { p_disabled.debug("Increment", increment(disabled_counter)); }
    PIKA_TEST_EQ(disabled_counter.load(), 0);

    // we do not expect the counter to increment: PIKA_DETAIL_DP_LAZY will not be evaluated
    p_disabled.debug("Increment", PIKA_DETAIL_DP_LAZY(p_disabled, increment(disabled_counter)));
    PIKA_TEST_EQ(disabled_counter.load(), 0);

    // ---------------------------------------------------------
    // Test that scoped log messages behave as expected
    {
        auto s_enabled = p_enabled.scope(
            "scoped block", PIKA_DETAIL_DP_LAZY(p_enabled, increment(enabled_counter)));
        [[maybe_unused]] auto s_disabled = p_disabled.scope(
            "scoped block", PIKA_DETAIL_DP_LAZY(p_disabled, increment(disabled_counter)));
    }
    PIKA_TEST_EQ(enabled_counter.load(), 3);
    PIKA_TEST_EQ(disabled_counter.load(), 0);

    // ---------------------------------------------------------
    // Test that debug only variables behave as expected
    // create high resolution timers to see if they count
    auto var1 =
        p_enabled.declare_variable<int>(PIKA_DETAIL_DP_LAZY(p_enabled, enabled_counter + 4));
    (void) var1;    // silenced unused var when optimized out

    auto var2 =
        p_disabled.declare_variable<int>(PIKA_DETAIL_DP_LAZY(p_disabled, disabled_counter + 10));
    (void) var2;    // silenced unused var when optimized out

    p_enabled.debug("var 1",
        pika::debug::detail::dec<>(PIKA_DETAIL_DP_LAZY(p_enabled, enabled_counter += var1)));
    p_disabled.debug("var 2",
        pika::debug::detail::dec<>(PIKA_DETAIL_DP_LAZY(p_disabled, disabled_counter += var2)));

    PIKA_TEST_EQ(enabled_counter.load(), 10);
    PIKA_TEST_EQ(disabled_counter.load(), 0);

    p_enabled.set(var1, 5);
    p_disabled.set(var2, 5);

    p_enabled.debug("var 1",
        pika::debug::detail::dec<>(PIKA_DETAIL_DP_LAZY(p_enabled, enabled_counter += var1)));
    p_disabled.debug("var 2",
        pika::debug::detail::dec<>(PIKA_DETAIL_DP_LAZY(p_disabled, disabled_counter += var2)));

    PIKA_TEST_EQ(enabled_counter.load(), 15);
    PIKA_TEST_EQ(disabled_counter.load(), 0);

    // ---------------------------------------------------------
    // Test that timed log messages behave as expected
    static auto t_enabled = p_enabled.make_timer(1, debug::detail::str<>("Timed (enabled)"));
    static auto t_disabled = p_disabled.make_timer(1, debug::detail::str<>("Timed (disabled)"));

    // run a loop for 2 seconds with a timed print every 1 sec
    auto start = std::chrono::system_clock::now();
    auto end = std::chrono::system_clock::now();
    while (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() < 2)
    {
        p_enabled.timed(t_enabled, "enabled",
            debug::detail::dec<3>(PIKA_DETAIL_DP_LAZY(p_enabled, ++enabled_counter)));

        p_disabled.timed(t_disabled, "disabled",
            debug::detail::dec<3>(PIKA_DETAIL_DP_LAZY(p_disabled, ++disabled_counter)));
        end = std::chrono::system_clock::now();
    }
    PIKA_TEST_EQ(enabled_counter.load() > 10, true);
    PIKA_TEST_EQ(disabled_counter.load(), 0);

    std::cout << "enabled  counter " << enabled_counter << std::endl;
    std::cout << "disabled counter " << disabled_counter << std::endl;

    return 0;
}
