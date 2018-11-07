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

#include <folly/logging/xlog.h>
#include <folly/String.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

static constexpr folly::StringPiece str4 = "42.42.12.34";
static constexpr folly::StringPiece str6 =
    "4242:4242:4242:4242:1234:1234:1234:1234";
static constexpr folly::StringPiece strMac = "42:42:42:12:34:56";

TEST(AddressUtilTest, IPAddress) {
  folly::IPAddress ip4(str4);
  sai_ip_address_t sai4 = toSaiIpAddress(ip4);
  folly::IPAddress reverse4 = fromSaiIpAddress(sai4);
  EXPECT_EQ(ip4, reverse4);
  folly::IPAddress ip6(str6);
  sai_ip_address_t sai6 = toSaiIpAddress(ip6);
  folly::IPAddress reverse6 = fromSaiIpAddress(sai6);
  EXPECT_EQ(ip6, reverse6);
}

TEST(AddressUtilTest, IPAddressV4) {
  folly::IPAddressV4 ip4(str4);
  sai_ip_address_t sai4 = toSaiIpAddress(ip4);
  folly::IPAddressV4 reverse4 = fromSaiIpAddress(sai4.addr.ip4);
  EXPECT_EQ(ip4, reverse4);
}

TEST(AddressUtilTest, IPAddressV6) {
  folly::IPAddressV6 ip6(str6);
  sai_ip_address_t sai6 = toSaiIpAddress(ip6);
  folly::IPAddressV6 reverse6 = fromSaiIpAddress(sai6.addr.ip6);
  EXPECT_EQ(ip6, reverse6);
}

TEST(AddressUtilTest, MacAddress) {
  folly::MacAddress mac(strMac);
  sai_mac_t saiMac;
  toSaiMacAddress(mac, saiMac);
  folly::MacAddress reverseMac = fromSaiMacAddress(saiMac);
  EXPECT_EQ(mac, reverseMac);
}
