/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "AddressUtil.h"

#include <folly/lang/Assume.h>

namespace facebook {
namespace fboss {

folly::IPAddress fromSaiIpAddress(const sai_ip_address_t& addr) {
  switch (addr.addr_family) {
    case SAI_IP_ADDR_FAMILY_IPV4:
      return fromSaiIpAddress(addr.addr.ip4);
    case SAI_IP_ADDR_FAMILY_IPV6:
      return fromSaiIpAddress(addr.addr.ip6);
  }
  folly::assume_unreachable();
}

folly::IPAddressV4 fromSaiIpAddress(const sai_ip4_t& ip4) {
  return folly::IPAddressV4::fromLong(ip4);
}
folly::IPAddressV6 fromSaiIpAddress(const sai_ip6_t& ip6) {
  return folly::IPAddressV6::fromBinary(folly::ByteRange(ip6, 16));
}

sai_ip_address_t toSaiIpAddress(const folly::IPAddress& addr) {
  if (addr.isV4()) {
    return toSaiIpAddress(addr.asV4());
  } else if (addr.isV6()) {
    return toSaiIpAddress(addr.asV6());
  }
  folly::assume_unreachable();
}
sai_ip_address_t toSaiIpAddress(const folly::IPAddressV4& addr) {
  sai_ip_address_t out;
  out.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
  out.addr.ip4 = addr.toLong();
  return out;
}
sai_ip_address_t toSaiIpAddress(const folly::IPAddressV6& addr) {
  sai_ip_address_t out;
  out.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
  folly::ByteRange r = addr.toBinary();
  std::copy(std::begin(r), std::end(r), std::begin(out.addr.ip6));
  return out;
}

folly::MacAddress fromSaiMacAddress(const sai_mac_t& mac) {
  return folly::MacAddress::fromBinary(
      folly::ByteRange(std::begin(mac), std::end(mac)));
}
void toSaiMacAddress(const folly::MacAddress& src, sai_mac_t& dst) {
  folly::ByteRange r(src.bytes(), 6);
  std::copy(std::begin(r), std::end(r), std::begin(dst));
}

} // namespace fboss
} // namespace facebook
