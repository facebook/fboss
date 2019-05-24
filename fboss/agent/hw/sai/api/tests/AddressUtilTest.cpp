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

#include <gtest/gtest.h>

#include <limits>
#include <vector>

using namespace facebook::fboss;

static constexpr folly::StringPiece str4 = "42.42.12.34";
static constexpr folly::StringPiece str6 =
    "4242:4242:4242:4242:1234:1234:1234:1234";
static constexpr folly::StringPiece strMac = "42:42:42:12:34:56";

class AddressUtilTest : public ::testing::Test {
 public:
  void SetUp() override {}
  folly::IPAddress ip4{str4};
  folly::CIDRNetwork net4{ip4, 24};
  folly::CIDRNetwork ones4{ip4, 32};
  folly::IPAddress ip6{str6};
  folly::CIDRNetwork net6{ip6, 64};
  folly::CIDRNetwork ones6{ip6, 128};
  folly::MacAddress mac{strMac};
};

TEST_F(AddressUtilTest, IPAddress) {
  sai_ip_address_t sai4 = toSaiIpAddress(ip4);
  folly::IPAddress reverse4 = fromSaiIpAddress(sai4);
  EXPECT_EQ(ip4, reverse4);
  sai_ip_address_t sai6 = toSaiIpAddress(ip6);
  folly::IPAddress reverse6 = fromSaiIpAddress(sai6);
  EXPECT_EQ(ip6, reverse6);
}

TEST_F(AddressUtilTest, IPAddressV4) {
  sai_ip_address_t sai4 = toSaiIpAddress(ip4);
  folly::IPAddressV4 reverse4 = fromSaiIpAddress(sai4.addr.ip4);
  EXPECT_EQ(ip4, reverse4);
}

TEST_F(AddressUtilTest, IPAddressV6) {
  sai_ip_address_t sai6 = toSaiIpAddress(ip6);
  folly::IPAddressV6 reverse6 = fromSaiIpAddress(sai6.addr.ip6);
  EXPECT_EQ(ip6, reverse6);
}

TEST_F(AddressUtilTest, PrefixV4) {
  sai_ip_prefix_t saiPrefix4 = toSaiIpPrefix(net4);
  EXPECT_EQ(saiPrefix4.addr_family, SAI_IP_ADDR_FAMILY_IPV4);
  folly::CIDRNetwork reverseNet4 = fromSaiIpPrefix(saiPrefix4);
  EXPECT_EQ(net4, reverseNet4);
}

TEST_F(AddressUtilTest, PrefixV6) {
  sai_ip_prefix_t saiPrefix6 = toSaiIpPrefix(net6);
  EXPECT_EQ(saiPrefix6.addr_family, SAI_IP_ADDR_FAMILY_IPV6);
  folly::CIDRNetwork reverseNet6 = fromSaiIpPrefix(saiPrefix6);
  EXPECT_EQ(net6, reverseNet6);
}

TEST_F(AddressUtilTest, PrefixAllOnes4) {
  sai_ip_prefix_t saiPrefix4 = toSaiIpPrefix(ones4);
  EXPECT_EQ(saiPrefix4.addr_family, SAI_IP_ADDR_FAMILY_IPV4);
  folly::CIDRNetwork reverseNet4 = fromSaiIpPrefix(saiPrefix4);
  EXPECT_EQ(ones4, reverseNet4);
}

TEST_F(AddressUtilTest, PrefixAllOnes6) {
  sai_ip_prefix_t saiPrefix6 = toSaiIpPrefix(ones6);
  EXPECT_EQ(saiPrefix6.addr_family, SAI_IP_ADDR_FAMILY_IPV6);
  folly::CIDRNetwork reverseNet6 = fromSaiIpPrefix(saiPrefix6);
  EXPECT_EQ(ones6, reverseNet6);
}

TEST_F(AddressUtilTest, MacAddress) {
  sai_mac_t saiMac;
  toSaiMacAddress(mac, saiMac);
  folly::MacAddress reverseMac = fromSaiMacAddress(saiMac);
  EXPECT_EQ(mac, reverseMac);
}
