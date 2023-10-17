//  Copyright (c) 2017 Mikael Simberg
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Simple test verifying basic resource_partitioner functionality.

#include <pika/init.hpp>
#include <pika/modules/resource_partitioner.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>
#include <pika/threading_base/thread_pool_base.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

int pika_main()
{
    // Try suspending without elasticity enabled, should throw an exception
    bool exception_thrown = false;

    try
    {
        pika::threads::detail::thread_pool_base& tp = pika::resource::get_thread_pool("default");

        // Use .get() to throw exception
        tp.suspend_processing_unit_direct(0);
        PIKA_TEST_MSG(false, "Suspending should not be allowed with elasticity disabled");
    }
    catch (pika::exception const&)
    {
        exception_thrown = true;
    }

    PIKA_TEST(exception_thrown);

    return pika::finalize();
}

int main(int argc, char* argv[])
{
    pika::init_params init_args;

    using ::pika::threads::scheduler_mode;
    init_args.cfg = {"pika.os_threads=" +
        std::to_string(((std::min)(
            std::size_t(4), std::size_t(pika::threads::detail::hardware_concurrency()))))};
    init_args.rp_callback = [](auto& rp, pika::program_options::variables_map const&) {
        // Explicitly disable elasticity if it is in defaults
        rp.create_thread_pool("default", pika::resource::scheduling_policy::local_priority_fifo,
            scheduler_mode::default_mode & ~scheduler_mode::enable_elasticity);
    };

    PIKA_TEST_EQ(pika::init(pika_main, argc, argv, init_args), 0);
}
