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
#include <folly/MacAddress.h>

extern "C" {
#include <sai.h>
}

/*
 * Utils for manipulating SAI network addresses
 * Note: sai_ip4_t, sai_ip6_t, and sai_mac_t are in network byte order
 */

namespace facebook::fboss {

folly::IPAddress fromSaiIpAddress(const sai_ip_address_t& addr);
folly::IPAddressV4 fromSaiIpAddress(const sai_ip4_t& addr);
folly::IPAddressV6 fromSaiIpAddress(const sai_ip6_t& addr);

sai_ip_address_t toSaiIpAddress(const folly::IPAddress& addr);
sai_ip_address_t toSaiIpAddress(const folly::IPAddressV4& addr);
sai_ip_address_t toSaiIpAddress(const folly::IPAddressV6& addr);

void toSaiIpAddressV6(const folly::IPAddressV6& addr, sai_ip6_t* ip6);

folly::CIDRNetwork fromSaiIpPrefix(const sai_ip_prefix_t& prefix);
sai_ip_prefix_t toSaiIpPrefix(const folly::CIDRNetwork& prefix);

folly::MacAddress fromSaiMacAddress(const sai_mac_t& mac);
void toSaiMacAddress(const folly::MacAddress& src, sai_mac_t& dst);

} // namespace facebook::fboss
