//  Copyright (c) 2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/datastructures/optional.hpp>
#include <pika/serialization/serialization_fwd.hpp>
#include <pika/serialization/serialize.hpp>

#include <utility>

namespace pika { namespace serialization {

    template <typename T>
    void save(output_archive& ar, pika::util::optional<T> const& o, unsigned)
    {
        bool const valid = bool(o);
        ar << valid;
        if (valid)
        {
            ar << *o;
        }
    }

    template <typename T>
    void load(input_archive& ar, pika::util::optional<T>& o, unsigned)
    {
        bool valid = false;
        ar >> valid;
        if (!valid)
        {
            o.reset();
            return;
        }

        T value;
        ar >> value;
        o.emplace(PIKA_MOVE(value));
    }

    PIKA_SERIALIZATION_SPLIT_FREE_TEMPLATE(
        (template <typename T>), (pika::util::optional<T>) )
}}    // namespace pika::serialization
