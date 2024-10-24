//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2015 Anton Bikineev
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file base_object.hpp

#pragma once

#include <pika/serialization/access.hpp>
#include <pika/serialization/serialization_fwd.hpp>
#include <pika/serialization/serialize.hpp>
#include <pika/serialization/traits/polymorphic_traits.hpp>

#include <type_traits>

namespace pika { namespace serialization {

    template <typename Derived, typename Base,
        typename Enable =
            typename pika::traits::is_intrusive_polymorphic<Derived>::type>
    struct base_object_type
    {
        base_object_type(Derived& d)
          : d_(d)
        {
        }
        Derived& d_;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            access::serialize(ar,
                static_cast<Base&>(
                    const_cast<typename std::decay<Derived>::type&>(d_)),
                0);
        }
    };

    // we need another specialization to explicitly
    // specify non-virtual calls of virtual functions in
    // intrusively serialized base classes.
    template <typename Derived, typename Base>
    struct base_object_type<Derived, Base, std::true_type>
    {
        base_object_type(Derived& d)
          : d_(d)
        {
        }
        Derived& d_;

        template <class Archive>
        void save(Archive& ar, unsigned) const
        {
            access::save_base_object(ar, static_cast<const Base&>(d_), 0);
        }

        template <class Archive>
        void load(Archive& ar, unsigned)
        {
            access::load_base_object(ar, static_cast<Base&>(d_), 0);
        }
        PIKA_SERIALIZATION_SPLIT_MEMBER();
    };

    template <typename Base, typename Derived>
    base_object_type<Derived, Base> base_object(Derived& d)
    {
        return base_object_type<Derived, Base>(d);
    }

    // allow our base_object_type to be serialized as prvalue
    // compiler should support good ADL implementation
    // but it is rather for all pika serialization library
    template <typename D, typename B>
    PIKA_FORCEINLINE output_archive& operator<<(
        output_archive& ar, base_object_type<D, B> t)
    {
        ar.invoke(t);
        return ar;
    }

    template <typename D, typename B>
    PIKA_FORCEINLINE input_archive& operator>>(
        input_archive& ar, base_object_type<D, B> t)
    {
        ar.invoke(t);
        return ar;
    }

    template <typename D, typename B>
    PIKA_FORCEINLINE output_archive& operator&(
        output_archive& ar, base_object_type<D, B> t)    //-V524
    {
        ar.invoke(t);
        return ar;
    }

    template <typename D, typename B>
    PIKA_FORCEINLINE input_archive& operator&(
        input_archive& ar, base_object_type<D, B> t)    //-V524
    {
        ar.invoke(t);
        return ar;
    }
}}    // namespace pika::serialization
