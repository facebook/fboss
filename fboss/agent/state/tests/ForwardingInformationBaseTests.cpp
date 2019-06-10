/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <gtest/gtest.h>
#include <memory>

namespace {

folly::IPAddressV4 ip4_0("0.0.0.0");
folly::IPAddressV4 ip4_48("48.0.0.0");
folly::IPAddressV4 ip4_64("64.0.0.0");
folly::IPAddressV4 ip4_72("72.0.0.0");
folly::IPAddressV4 ip4_80("80.0.0.0");
folly::IPAddressV4 ip4_128("128.0.0.0");
folly::IPAddressV4 ip4_160("160.0.0.0");

folly::IPAddressV6 ip6_0("::");
folly::IPAddressV6 ip6_48("3000::");
folly::IPAddressV6 ip6_64("4000::");
folly::IPAddressV6 ip6_72("4800::");
folly::IPAddressV6 ip6_80("5000::");
folly::IPAddressV6 ip6_128("8000::");
folly::IPAddressV6 ip6_160("A000::");

template <typename AddressT>
std::shared_ptr<facebook::fboss::Route<AddressT>> createRouteFromPrefix(
    AddressT address,
    uint8_t mask) {
  facebook::fboss::RoutePrefix<AddressT> routePrefix{address, mask};
  facebook::fboss::RouteFields fields(routePrefix);

  return std::make_shared<facebook::fboss::Route<AddressT>>(fields);
}

class ForwardingInformationBaseV4Test : public ::testing::Test {
 protected:
  void SetUp() override {
    // The following prefixes are inserted into the FIB:
    // 0
    // 0000
    // 00110
    // 010
    // 01001
    // 0101
    // 10
    // 101
    fib.addNode(createRouteFromPrefix(ip4_0, 1));
    fib.addNode(createRouteFromPrefix(ip4_128, 2));
    fib.addNode(createRouteFromPrefix(ip4_64, 3));
    fib.addNode(createRouteFromPrefix(ip4_160, 3));
    fib.addNode(createRouteFromPrefix(ip4_80, 4));
    fib.addNode(createRouteFromPrefix(ip4_0, 4));
    fib.addNode(createRouteFromPrefix(ip4_48, 5));
    fib.addNode(createRouteFromPrefix(ip4_72, 6));
  }

  facebook::fboss::ForwardingInformationBaseV4 fib;
};

class ForwardingInformationBaseV6Test : public ::testing::Test {
 protected:
  void SetUp() override {
    // The following prefixes are inserted into the FIB:
    // 0
    // 0000
    // 00110
    // 010
    // 01001
    // 0101
    // 10
    // 101
    fib.addNode(createRouteFromPrefix(ip6_0, 1));
    fib.addNode(createRouteFromPrefix(ip6_128, 2));
    fib.addNode(createRouteFromPrefix(ip6_64, 3));
    fib.addNode(createRouteFromPrefix(ip6_160, 3));
    fib.addNode(createRouteFromPrefix(ip6_80, 4));
    fib.addNode(createRouteFromPrefix(ip6_0, 4));
    fib.addNode(createRouteFromPrefix(ip6_48, 5));
    fib.addNode(createRouteFromPrefix(ip6_72, 6));
  }

  facebook::fboss::ForwardingInformationBaseV6 fib;
};

template <typename AddressT>
void CHECK_LPM(
    const std::shared_ptr<facebook::fboss::Route<AddressT>>& route,
    AddressT address,
    uint8_t mask) {
  facebook::fboss::RoutePrefix<AddressT> routePrefix{address, mask};
  facebook::fboss::RouteFields fields(routePrefix);

  EXPECT_EQ(route->prefix().network, address);
  EXPECT_EQ(route->prefix().mask, mask);
}

} // namespace

namespace facebook {
namespace fboss {

TEST_F(ForwardingInformationBaseV4Test, SparseLPM) {
  // Candidate prefixes: 0/1, 0/4
  CHECK_LPM(fib.longestMatch(folly::IPAddressV4("0.0.0.0")), ip4_0, 4);

  // Candidate prefixes: 0/1, 64/3
  CHECK_LPM(fib.longestMatch(folly::IPAddressV4("64.1.0.1")), ip4_64, 3);

  // Candidate prefixes: 128/2, 160/3
  CHECK_LPM(fib.longestMatch(folly::IPAddressV4("161.16.8.1")), ip4_160, 3);
}

TEST_F(ForwardingInformationBaseV6Test, SparseLPM) {
  // Candidate prefixes: ::/1, ::/4
  CHECK_LPM(fib.longestMatch(folly::IPAddressV6("::")), ip6_0, 4);

  // Candidate prefixes: ::/1, 4000::/3
  CHECK_LPM(fib.longestMatch(folly::IPAddressV6("4001:1::")), ip6_64, 3);

  // Candidate prefixes: 8000::/2, A000::/3
  CHECK_LPM(fib.longestMatch(folly::IPAddressV6("A110:801::")), ip6_160, 3);
}

TEST_F(ForwardingInformationBaseV4Test, LPMDoesNotExist) {
  folly::IPAddressV4 address("192.0.0.0");

  EXPECT_EQ(nullptr, fib.longestMatch(address));
}

TEST_F(ForwardingInformationBaseV6Test, LPMDoesNotExist) {
  folly::IPAddressV6 address("C000::");

  EXPECT_EQ(nullptr, fib.longestMatch(address));
}

TEST_F(ForwardingInformationBaseV4Test, IncreasingLPMSequence) {
  folly::IPAddressV4 address("255.255.255.255");
  for (uint8_t mask = 0; mask <= address.bitCount(); ++mask) {
    auto addressWithCurrentMask = address.mask(mask);
    fib.addNode(createRouteFromPrefix(addressWithCurrentMask, mask));
    CHECK_LPM(fib.longestMatch(address), addressWithCurrentMask, mask);
  }
}

TEST_F(ForwardingInformationBaseV6Test, IncreasingLPMSequence) {
  folly::IPAddressV6 address("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF");
  for (uint16_t mask = 0; mask <= address.bitCount(); ++mask) {
    auto addressWithCurrentMask = address.mask(mask);
    fib.addNode(createRouteFromPrefix(addressWithCurrentMask, mask));
    CHECK_LPM(fib.longestMatch(address), addressWithCurrentMask, mask);
  }
}

TEST_F(ForwardingInformationBaseV4Test, DecreasingLPMSequence) {
  folly::IPAddressV4 address("255.255.255.255");
  for (int8_t mask = address.bitCount(); mask >= 0; --mask) {
    auto addressWithCurrentMask = address.mask(mask);
    fib.addNode(createRouteFromPrefix(addressWithCurrentMask, mask));
    CHECK_LPM(fib.longestMatch(address), address, address.bitCount());
  }
}

TEST_F(ForwardingInformationBaseV6Test, DecreasingLPMSequence) {
  folly::IPAddressV6 address("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF");
  for (int16_t mask = address.bitCount(); mask >= 0; --mask) {
    auto addressWithCurrentMask = address.mask(mask);
    fib.addNode(createRouteFromPrefix(addressWithCurrentMask, mask));
    CHECK_LPM(fib.longestMatch(address), address, address.bitCount());
  }
}

} // namespace fboss
} // namespace facebook
