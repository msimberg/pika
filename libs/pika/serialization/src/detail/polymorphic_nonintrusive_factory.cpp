//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2015 Andreas Schaefer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <pika/serialization/detail/polymorphic_nonintrusive_factory.hpp>

namespace pika { namespace serialization { namespace detail {
    polymorphic_nonintrusive_factory&
    polymorphic_nonintrusive_factory::instance()
    {
        pika::util::static_<polymorphic_nonintrusive_factory> factory;
        return factory.get();
    }
}}}    // namespace pika::serialization::detail
