//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The purpose of this demo is to show how it is possible to start pika without
// blocking the main thread.

#include <pika/local/init.hpp>

#include <iostream>

// Our main pika thread does nothing except for signalling to the runtime to
// finalize.
int pika_main()
{
    return pika::local::finalize();
}

int main(int argc, char* argv[])
{
    pika::local::start(pika_main, argc, argv);

    std::cout << "Hello from the main thread!" << std::endl;

    return pika::local::stop();
}
