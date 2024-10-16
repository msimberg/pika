//  Copyright (c) 2015 Thomas Heller
//  Copyright (c) 2015 Anton Bikineev
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/serialization/base_object.hpp>
#include <pika/serialization/input_archive.hpp>
#include <pika/serialization/output_archive.hpp>
#include <pika/serialization/serialize.hpp>

#include <pika/modules/testing.hpp>

#include <cstddef>
#include <string>
#include <vector>

template <typename T>
struct Base
{
    Base() = default;

    explicit Base(std::string const& prefix)
      : prefix_(prefix)
    {
    }

    virtual ~Base() {}

    virtual std::size_t size() = 0;
    virtual std::string print() = 0;

    std::string prefix_;
};

PIKA_TRAITS_NONINTRUSIVE_POLYMORPHIC_TEMPLATE((template <class T>), Base<T>)

template <typename Archive, typename T>
void serialize(Archive& ar, Base<T>& b, unsigned)
{
    ar& b.prefix_;
}

template <typename T>
struct Derived1 : Base<T>
{
    Derived1()
      : size_(0)
    {
    }

    Derived1(std::string const& prefix, std::size_t size)
      : Base<T>(prefix)
      , size_(size)
    {
    }

    std::size_t size() override
    {
        return size_;
    }

    std::size_t size_;
};

PIKA_TRAITS_NONINTRUSIVE_POLYMORPHIC_TEMPLATE((template <class T>), Derived1<T>)

template <typename Archive, typename T>
void serialize(Archive& ar, Derived1<T>& d1, unsigned)
{
    ar& pika::serialization::base_object<Base<T>>(d1);
    ar& d1.size_;
}

struct Derived2 : Derived1<double>
{
    Derived2() {}

    Derived2(
        std::string const& message, std::string const& prefix, std::size_t size)
      : Derived1<double>(prefix, size)
      , message_(message)
    {
    }

    std::string print() override
    {
        return message_;
    }

    std::string message_;
};

PIKA_SERIALIZATION_REGISTER_CLASS(Derived2)

template <typename Archive>
void serialize(Archive& ar, Derived2& d2, unsigned)
{
    ar& pika::serialization::base_object<Derived1<double>>(d2);
    ar& d2.message_;
}

int main()
{
    std::vector<char> buffer;
    Derived2 d("/tmp", "fancy", 10);
    Base<double>& b = d;
    Base<double>* b_ptr = &d;
    PIKA_TEST_EQ(d.print(), b.print());
    PIKA_TEST_EQ(d.size(), b.size());
    PIKA_TEST_EQ(d.prefix_, b.prefix_);
    PIKA_TEST_EQ(d.print(), b_ptr->print());
    PIKA_TEST_EQ(d.size(), b_ptr->size());
    PIKA_TEST_EQ(d.prefix_, b_ptr->prefix_);

    {
        pika::serialization::output_archive oarchive(buffer);
        oarchive << b;
        oarchive << pika::serialization::detail::raw_ptr(b_ptr);
    }

    {
        Derived2 d1;
        Derived2 d2;
        Base<double>& b1 = d1;
        Base<double>* b2 = nullptr;

        pika::serialization::input_archive iarchive(buffer);
        iarchive >> b1;
        iarchive >> pika::serialization::detail::raw_ptr(b2);

        PIKA_TEST(b2 != nullptr);

        PIKA_TEST_EQ(d.print(), b1.print());
        PIKA_TEST_EQ(d.size(), b1.size());
        PIKA_TEST_EQ(d.prefix_, b1.prefix_);
        PIKA_TEST_EQ(d.print(), b2->print());
        PIKA_TEST_EQ(d.size(), b2->size());
        PIKA_TEST_EQ(d.prefix_, b2->prefix_);

        delete b2;
    }
    return pika::util::report_errors();
}
