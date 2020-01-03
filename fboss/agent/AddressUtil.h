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
#include <folly/IPAddress.h>
#include "common/network/if/gen-cpp2/Address_types.h"

namespace facebook::network {

// Trait for V4 vs V6
template <typename T>
struct IPVersion;
template <>
struct IPVersion<folly::IPAddressV4> {
  static const thrift::AddressType value = thrift::AddressType::V4;
};
template <>
struct IPVersion<folly::IPAddressV6> {
  static const thrift::AddressType value = thrift::AddressType::V6;
};

template <class T>
thrift::Address toAddressImpl(const T& addr) {
  thrift::Address result;
  result.addr = addr.toFullyQualified();
  result.type = IPVersion<T>::value;
  return result;
}

inline thrift::Address toAddress(const folly::IPAddress& ip) {
  return ip.isV4() ? toAddressImpl(ip.asV4())
                   : ip.isV6() ? toAddressImpl(ip.asV6()) : thrift::Address();
}

template <class IPAddressVx>
thrift::BinaryAddress toBinaryAddressImpl(const IPAddressVx& addr) {
  thrift::BinaryAddress result;
  result.addr.append(
      reinterpret_cast<const char*>(addr.bytes()), IPAddressVx::byteCount());
  return result;
}

inline thrift::BinaryAddress toBinaryAddress(const folly::IPAddress& addr) {
  return addr.isV4() ? toBinaryAddressImpl(addr.asV4())
                     : addr.isV6() ? toBinaryAddressImpl(addr.asV6())
                                   : thrift::BinaryAddress();
}

template <typename T>
inline folly::IPAddress toIPAddress(const T& input) {
  return input.type != decltype(input.type)::VUNSPEC
      ? folly::IPAddress(input.addr)
      : folly::IPAddress();
}

inline folly::IPAddress toIPAddress(const thrift::BinaryAddress& addr) {
  return folly::IPAddress::fromBinary(folly::ByteRange(
      reinterpret_cast<const unsigned char*>(addr.addr.data()),
      addr.addr.size()));
}

} // namespace facebook::network
