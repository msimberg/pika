//  Copyright (c) 2005-2017 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

PIKA_GLOBAL_MODULE_FRAGMENT

#include <pika/config.hpp>

#if !defined(PIKA_HAVE_MODULE)
#include <pika/schedulers/deadlock_detection.hpp>
#endif

#if defined(PIKA_HAVE_MODULE)
module pika.schedulers;
#endif

namespace pika::threads::detail {
#ifdef PIKA_HAVE_THREAD_DEADLOCK_DETECTION
    static bool deadlock_detection_enabled = false;

    void set_deadlock_detection_enabled(bool enabled) { deadlock_detection_enabled = enabled; }

    bool get_deadlock_detection_enabled() { return deadlock_detection_enabled; }
#endif
}    // namespace pika::threads::detail
