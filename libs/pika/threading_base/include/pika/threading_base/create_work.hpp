//  Copyright (c) 2007-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/threading_base/thread_init_data.hpp>
#include <pika/threading_base/threading_base_fwd.hpp>

#if !defined(PIKA_HAVE_MODULE)
#include <pika/modules/errors.hpp>
#endif

namespace pika::threads::detail {
    PIKA_EXPORT thread_id_ref_type create_work(
        scheduler_base* scheduler, thread_init_data& data, error_code& ec = throws);
}    // namespace pika::threads::detail
