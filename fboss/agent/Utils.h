/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <type_traits> // To use 'std::integral_constant'.
#include <folly/Bits.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/Range.h>
#include "fboss/agent/types.h"

namespace facebook { namespace fboss {

class SwitchState;

template<typename T>
inline T readBuffer(const uint8_t* buffer, uint32_t pos, size_t buffSize) {
  CHECK_LE(pos + sizeof(T), buffSize);
  T tmp;
  memcpy(&tmp, buffer + pos, sizeof(tmp));
  return folly::Endian::big(tmp);
}

/*
 * Create a directory, recursively creating all parents if necessary.
 *
 * If the directory already exists this function is a no-op.
 * Throws an exception on error.
 */
void utilCreateDir(folly::StringPiece path);

/**
 * Helper function to get an IPv4 address for a particular vlan
 * Used to set src IP address for DHCP and ICMP packets
 * throw an FbossError in case no IP address exists.
 */
folly::IPAddressV4 getSwitchVlanIP(const std::shared_ptr<SwitchState>& state,
                                   VlanID vlan);

/**
 * Helper function to get an IPv6 address for a particular vlan
 * used to set src IP address for DHCPv6 and ICMPv6 packets
 * throw an FbossError in case no IPv6 address exists.
 */
folly::IPAddressV6 getSwitchVlanIPv6(const std::shared_ptr<SwitchState>& state,
                                     VlanID vlan);

/*
 * C++ member detector. Adapted from - i
 * http://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
 * Goal here is to detect whether a type T has a T::member
 * 1. Define a class HasMember_member with 2 inner types
 * Fallback which defines a member variable called member (viz. Forward::member)
 * Derived - which is an empty class derived from Fallback and T. Now if
 * there exists T::member then Derived::member would be ambiguous.
 * 2. Define 2 template member functions named test where one takes parameter of
 * type U::member* as argument and other takes U* as argument. The return values
 * from these functions are an char arrya of size 1 (called No) and a char array
 * of size 2 (called Yes).
 * 3. Evaluate
 * RESULT = sizeof(test<Derived>(nullptr)) == sizeof(Yes);
 * This essentially tells us which of the 2 test instances would be instantiated
 * as a result of this expression. Key point to note here is that if T::member
 * exists then Derived::member is ambiguous and the first test declaration is
 * omitted, else the first test declaration would be picked. We check the return
 * value size to know which got picked.
 *
 * Finally has_member_##member is defined to provide familiar access via
 * has_member_##member::value. The additional var args parameter to this class
 * is for situations where we want to use this in a situation where we have
 * a type T for which we want to determine whether T::member exists but T is
 * not a template parameter in this context. In this case we can add a dummy
 * variadic template arguments list of size 0 and use this querying capability.
 * Usage:
 * For detecting whether member bar exists
 * GENERATE_HAS_MEMBER(bar);
 * This will generate 2 class templates
 * <typename T> HasMember_bar {...}and <typename T> has_member_bar { ... };
 * Now to determine whether Foo::bar exists simply use
 * has_member_bar<Foo>::value OR
 * has_member_bar<Foo>()
 */
#define GENERATE_HAS_MEMBER(member)                                         \
                                                                            \
template <typename T >                                                      \
class HasMember_##member                                                    \
{                                                                           \
private:                                                                    \
    using Yes = char[2];                                                    \
    using  No = char[1];                                                    \
                                                                            \
    struct Fallback { int member; };                                        \
    struct Derived : T, Fallback { };                                       \
                                                                            \
    template <class U>                                                      \
    static No& test ( decltype(U::member)* );                               \
    template <typename U>                                                   \
    static Yes& test (U* );                                                 \
                                                                            \
public:                                                                     \
    static constexpr bool RESULT =                                          \
          sizeof(test<Derived>(nullptr)) == sizeof(Yes);                    \
};                                                                          \
                                                                            \
template <typename T, typename... Args>                                     \
struct has_member_##member                                                  \
: public std::integral_constant<bool, HasMember_##member<T>::RESULT>        \
{                                                                           \
  static_assert(sizeof ...(Args) == 0, "Args list must be empty");          \
};                                                                          \

}}
