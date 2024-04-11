//  Copyright (c) 2007-2021 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/logging.hpp>
#include <pika/runtime_configuration/runtime_configuration.hpp>

#include <string>

namespace pika::detail {
    /// \cond NOINTERNAL
    struct log_settings
    {
        std::string level_;
        std::string dest_;
        std::string format_;
    };

    PIKA_EXPORT log_settings get_log_settings(pika::detail::section const&, char const*);
    PIKA_EXPORT void init_logging(pika::util::runtime_configuration& ini);

    /// \endcond

    PIKA_EXPORT void enable_logging(
        std::string const& lvl = "5", std::string logdest = "", std::string logformat = "");
    PIKA_EXPORT void disable_logging();
}    // namespace pika::detail
