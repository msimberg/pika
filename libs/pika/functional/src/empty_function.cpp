//  Copyright (c) 2011 Thomas Heller
//  Copyright (c) 2013 Hartmut Kaiser
//  Copyright (c) 2014 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(PIKA_HAVE_MODULE)
module;
#endif

#if !defined(PIKA_HAVE_MODULE)
#include <pika/functional/detail/empty_function.hpp>
#include <pika/modules/errors.hpp>
#endif

#if defined(PIKA_HAVE_MODULE)
module pika.functional;
#endif

namespace pika::util::detail {
    [[noreturn]] void throw_bad_function_call()
    {
        pika::throw_exception(pika::error::bad_function_call,
            "empty function object should not be used", "empty_function::operator()");
    }
}    // namespace pika::util::detail
