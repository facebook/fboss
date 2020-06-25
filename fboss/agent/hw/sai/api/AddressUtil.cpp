/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include <folly/String.h>

#include <folly/logging/xlog.h>

namespace {
constexpr auto kFullMaskV4 = folly::StringPiece("255.255.255.255");
constexpr auto kFullMaskV6 =
    folly::StringPiece("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF");

// To calculate the mask length for an IPAddress representing a mask,
// (assumming that the mask address is in network byte order and
// takes the form 1111...0000) start at the most significant bit and walk
// towards the least significant bit until we find a 0.
uint8_t calculateMaskLengthFromIPAsMask(const folly::IPAddress& mask) {
  size_t bit = 0;
  while (bit < mask.bitCount()) {
    if (!mask.getNthMSBit(bit)) {
      break;
    }
    ++bit;
  }
  return bit;
}
} // namespace

#include <folly/lang/Assume.h>

namespace facebook::fboss {

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

void toSaiIpAddressV6(const folly::IPAddressV6& addr, sai_ip6_t* ip6) {
  folly::ByteRange r = addr.toBinary();
  std::copy(std::begin(r), std::end(r), std::begin(*ip6));
}

folly::CIDRNetwork fromSaiIpPrefix(const sai_ip_prefix_t& prefix) {
  folly::IPAddress outAddr;
  folly::IPAddress mask;
  uint8_t maskLength;
  switch (prefix.addr_family) {
    case SAI_IP_ADDR_FAMILY_IPV4:
      outAddr = fromSaiIpAddress(prefix.addr.ip4);
      mask = fromSaiIpAddress(prefix.mask.ip4);
      maskLength = calculateMaskLengthFromIPAsMask(mask);
      break;
    case SAI_IP_ADDR_FAMILY_IPV6:
      outAddr = fromSaiIpAddress(prefix.addr.ip6);
      mask = fromSaiIpAddress(prefix.mask.ip6);
      maskLength = calculateMaskLengthFromIPAsMask(mask);
      break;
    default:
      // error
      break;
  }
  return {outAddr, maskLength};
}

sai_ip_prefix_t toSaiIpPrefix(const folly::CIDRNetwork& prefix) {
  sai_ip_prefix_t out;
  bool v4 = prefix.first.isV4();
  folly::IPAddress mask =
      (v4 ? folly::IPAddress(kFullMaskV4) : folly::IPAddress(kFullMaskV6))
          .mask(prefix.second);
  sai_ip_address_t saiAddr = toSaiIpAddress(prefix.first);
  sai_ip_address_t saiMask = toSaiIpAddress(mask);
  out.addr_family = saiAddr.addr_family;
  if (v4) {
    out.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    out.addr.ip4 = saiAddr.addr.ip4;
    out.mask.ip4 = saiMask.addr.ip4;
  } else {
    out.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    std::copy(saiAddr.addr.ip6, saiAddr.addr.ip6 + 16, out.addr.ip6);
    std::copy(saiMask.addr.ip6, saiMask.addr.ip6 + 16, out.mask.ip6);
  }
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

} // namespace facebook::fboss
