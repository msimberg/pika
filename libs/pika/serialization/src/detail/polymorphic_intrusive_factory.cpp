//  Copyright (c) 2014 Anton Bikineev
//  Copyright (c) 2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <pika/serialization/detail/polymorphic_intrusive_factory.hpp>

#include <pika/config.hpp>
#include <pika/modules/errors.hpp>
#include <pika/type_support/static.hpp>

#include <string>

namespace pika { namespace serialization { namespace detail {
    polymorphic_intrusive_factory& polymorphic_intrusive_factory::instance()
    {
        pika::util::static_<polymorphic_intrusive_factory> factory;
        return factory.get();
    }

    void polymorphic_intrusive_factory::register_class(
        std::string const& name, ctor_type fun)
    {
        if (name.empty())
        {
            PIKA_THROW_EXCEPTION(serialization_error,
                "polymorphic_intrusive_factory::register_class",
                "Cannot register a factory with an empty name");
        }

        auto it = map_.find(name);
        if (it == map_.end())
        {
            map_.emplace(name, fun);
        }
    }

    void* polymorphic_intrusive_factory::create(std::string const& name) const
    {
        return map_.at(name)();
    }
}}}    // namespace pika::serialization::detail