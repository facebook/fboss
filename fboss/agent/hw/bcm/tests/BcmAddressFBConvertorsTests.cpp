/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/tests/BcmUnitTestUtils.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>

#include <gtest/gtest.h>
extern "C" {
#include <bcm/types.h>
}

extern "C" {
struct ibde_t;
ibde_t* bde;
}

using namespace facebook::fboss;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;

TEST(MacConversion, toFromBcm) {
  auto mac = MacAddress("11:22:33:44:55:66");
  bcm_mac_t bcmMac;
  macToBcm(mac, &bcmMac);
  EXPECT_EQ(mac, macFromBcm(bcmMac));
  MacAddress newMac;
  macFromBcm(bcmMac, &newMac);
  EXPECT_EQ(mac, newMac);
}

TEST(V4Bcm, toBcm6) {
  auto ip = IPAddress(IPAddressV4("10.10.0.0"));
  bcm_ip6_t bcmIp;
  ipToBcmIp6(ip, &bcmIp);
  std::array<uint8_t, sizeof(bcm_ip6_t)> expectedBcmIp = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 10, 0, 0};
  EXPECT_EQ(16, sizeof(bcm_ip6_t));
  EXPECT_EQ(0, memcmp(bcmIp, expectedBcmIp.data(), sizeof(bcm_ip6_t)));
}

TEST(V4Bcm, fromBcm6) {
  auto ip = IPAddress(IPAddressV4("10.10.0.0"));
  bcm_ip6_t bcmIp;
  ipToBcmIp6(ip, &bcmIp);
  auto ipPostConversion = ipFromBcm(bcmIp);
  // Since we convert IPAddress to bcm_ip6_t, we lose the knowledge
  // that this was a v4 IP and will thus read it back as a v6 IP with
  // the lowest 4 bytes from the original IP
  EXPECT_NE(ip, ipPostConversion);
  EXPECT_EQ(IPAddress("::10.10.0.0"), ipPostConversion);
}

TEST(V6Bcm, toBcm6) {
  auto ip = IPAddress(IPAddressV6("ff02::"));
  bcm_ip6_t bcmIp;
  ipToBcmIp6(ip, &bcmIp);
  std::array<uint8_t, sizeof(bcm_ip6_t)> expectedBcmIp = {
      0xff, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  EXPECT_EQ(16, sizeof(bcm_ip6_t));
  EXPECT_EQ(0, memcmp(bcmIp, expectedBcmIp.data(), sizeof(bcm_ip6_t)));
}

TEST(V6Bcm, fromBcm6) {
  auto ip = IPAddress(IPAddressV6("ff02::"));
  bcm_ip6_t bcmIp;
  ipToBcmIp6(ip, &bcmIp);
  auto ipPostConversion = ipFromBcm(bcmIp);
  EXPECT_EQ(ip, ipPostConversion);
}
