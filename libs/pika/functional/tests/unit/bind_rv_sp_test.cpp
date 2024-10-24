//  Taken from the Boost.Bind library
//
//  bind_rv_sp_test.cpp - smart pointer returned by value from an inner bind
//
//  Copyright (c) 2005 Peter Dimov
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(PIKA_MSVC)
#pragma warning(disable : 4786)    // identifier truncated in debug info
#pragma warning(disable : 4710)    // function not inlined
#pragma warning(                                                               \
    disable : 4711)    // function selected for automatic inline expansion
#pragma warning(disable : 4514)    // unreferenced inline removed
#endif

#include <pika/functional/bind.hpp>

namespace placeholders = pika::util::placeholders;

#include <iostream>
#include <memory>

#include <pika/modules/testing.hpp>

struct X
{
    int v_;

    X(int v)
      : v_(v)
    {
    }

    int f()
    {
        return v_;
    }
};

struct Y
{
    std::shared_ptr<X> f()
    {
        return std::shared_ptr<X>(new X(42));
    }
};

int main()
{
    Y y;

    PIKA_TEST_EQ(pika::util::bind(&X::f, pika::util::bind(&Y::f, &y))(), 42);

    return pika::util::report_errors();
}
