/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Utils.h"
#include "fboss/agent/rib/FibUpdateHelpers.h"
#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/RoutingInformationBase.h"

#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <gtest/gtest.h>
#include <memory>

using namespace facebook::fboss;

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

const auto kRid0 = RouterID(0);

template <typename AddressT>
void addRoute(
    NetworkToRouteMap<AddressT>& rib,
    const std::shared_ptr<Route<AddressT>>& route) {
  rib.insert(route->prefix().network, route->prefix().mask, route);
}

void addRoute(RoutingInformationBase& rib, const UnicastRoute& route) {
  rib.update(
      nullptr,
      kRid0,
      ClientID::BGPD,
      AdminDistance::EBGP,
      {route},
      {},
      false,
      "Rib only update",
      noopFibUpdate,
      nullptr);
}

class V4LpmTest : public ::testing::Test {
 protected:
  void SetUp() override {
    rib.ensureVrf(kRid0);
    // The following prefixes are inserted into the FIB:
    // 0
    // 0000
    // 00110
    // 010
    // 01001
    // 0101
    // 10
    // 101
    addRoute(rib, makeDropUnicastRoute({ip4_0, 1}));
    addRoute(rib, makeDropUnicastRoute({ip4_128, 2}));
    addRoute(rib, makeDropUnicastRoute({ip4_64, 3}));
    addRoute(rib, makeDropUnicastRoute({ip4_160, 3}));
    addRoute(rib, makeDropUnicastRoute({ip4_80, 4}));
    addRoute(rib, makeDropUnicastRoute({ip4_0, 4}));
    addRoute(rib, makeDropUnicastRoute({ip4_48, 5}));
    addRoute(rib, makeDropUnicastRoute({ip4_72, 6}));
  }

  std::shared_ptr<Route<folly::IPAddressV4>> longestMatch(
      folly::IPAddressV4 addr) {
    return rib.longestMatch(addr, kRid0);
  }

  RoutingInformationBase rib;
};

class V6LpmTest : public ::testing::Test {
 protected:
  void SetUp() override {
    rib.ensureVrf(kRid0);
    // The following prefixes are inserted into the FIB:
    // 0
    // 0000
    // 00110
    // 010
    // 01001
    // 0101
    // 10
    // 101
    addRoute(rib, makeDropUnicastRoute({ip6_0, 1}));
    addRoute(rib, makeDropUnicastRoute({ip6_128, 2}));
    addRoute(rib, makeDropUnicastRoute({ip6_64, 3}));
    addRoute(rib, makeDropUnicastRoute({ip6_160, 3}));
    addRoute(rib, makeDropUnicastRoute({ip6_80, 4}));
    addRoute(rib, makeDropUnicastRoute({ip6_0, 4}));
    addRoute(rib, makeDropUnicastRoute({ip6_48, 5}));
    addRoute(rib, makeDropUnicastRoute({ip6_72, 6}));
  }
  std::shared_ptr<Route<folly::IPAddressV6>> longestMatch(
      folly::IPAddressV6 addr) {
    return rib.longestMatch(addr, kRid0);
  }

  RoutingInformationBase rib;
};

template <typename AddressT>
void CHECK_LPM(
    const std::shared_ptr<facebook::fboss::Route<AddressT>>& route,
    AddressT address,
    uint8_t mask) {
  facebook::fboss::RoutePrefix<AddressT> routePrefix{address, mask};
  facebook::fboss::RouteFields<AddressT> fields(routePrefix);

  EXPECT_EQ(route->prefix().network(), address);
  EXPECT_EQ(route->prefix().mask(), mask);
}

TEST_F(V4LpmTest, SparseLPM) {
  // Candidate prefixes: 0/1, 0/4
  CHECK_LPM(longestMatch(folly::IPAddressV4("0.0.0.0")), ip4_0, 4);

  // Candidate prefixes: 0/1, 64/3
  CHECK_LPM(longestMatch(folly::IPAddressV4("64.1.0.1")), ip4_64, 3);

  // Candidate prefixes: 128/2, 160/3
  CHECK_LPM(longestMatch(folly::IPAddressV4("161.16.8.1")), ip4_160, 3);
}

TEST_F(V6LpmTest, SparseLPM) {
  // Candidate prefixes: ::/1, ::/4
  CHECK_LPM(longestMatch(folly::IPAddressV6("::")), ip6_0, 4);

  // Candidate prefixes: ::/1, 4000::/3
  CHECK_LPM(longestMatch(folly::IPAddressV6("4001:1::")), ip6_64, 3);

  // Candidate prefixes: 8000::/2, A000::/3
  CHECK_LPM(longestMatch(folly::IPAddressV6("A110:801::")), ip6_160, 3);
}

TEST_F(V4LpmTest, LPMDoesNotExist) {
  folly::IPAddressV4 address("192.0.0.0");

  EXPECT_EQ(nullptr, longestMatch(address));
}

TEST_F(V6LpmTest, LPMDoesNotExist) {
  folly::IPAddressV6 address("C000::");

  EXPECT_EQ(nullptr, longestMatch(address));
}

TEST_F(V4LpmTest, IncreasingLPMSequence) {
  folly::IPAddressV4 address("255.255.255.255");
  for (uint8_t mask = 0; mask <= address.bitCount(); ++mask) {
    auto addressWithCurrentMask = address.mask(mask);
    addRoute(rib, makeDropUnicastRoute({addressWithCurrentMask, mask}));
    CHECK_LPM(longestMatch(address), addressWithCurrentMask, mask);
  }
  auto ribBack = RoutingInformationBase::fromThrift(rib.toThrift());
  EXPECT_EQ(ribBack->toThrift(), rib.toThrift());
}

TEST_F(V6LpmTest, IncreasingLPMSequence) {
  folly::IPAddressV6 address("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF");
  for (uint16_t mask = 0; mask <= address.bitCount(); ++mask) {
    auto addressWithCurrentMask = address.mask(mask);
    addRoute(rib, makeDropUnicastRoute({addressWithCurrentMask, mask}));
    CHECK_LPM(longestMatch(address), addressWithCurrentMask, mask);
  }
  auto ribBack = RoutingInformationBase::fromThrift(rib.toThrift());
  EXPECT_EQ(ribBack->toThrift(), rib.toThrift());
}

TEST_F(V4LpmTest, DecreasingLPMSequence) {
  folly::IPAddressV4 address("255.255.255.255");
  for (int8_t mask = address.bitCount(); mask >= 0; --mask) {
    auto addressWithCurrentMask = address.mask(mask);
    addRoute(rib, makeDropUnicastRoute({addressWithCurrentMask, mask}));
    CHECK_LPM(longestMatch(address), address, address.bitCount());
  }
  auto ribBack = RoutingInformationBase::fromThrift(rib.toThrift());
  EXPECT_EQ(ribBack->toThrift(), rib.toThrift());
}

TEST_F(V6LpmTest, DecreasingLPMSequence) {
  folly::IPAddressV6 address("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF");
  for (int16_t mask = address.bitCount(); mask >= 0; --mask) {
    auto addressWithCurrentMask = address.mask(mask);
    addRoute(rib, makeDropUnicastRoute({addressWithCurrentMask, mask}));
    CHECK_LPM(longestMatch(address), address, address.bitCount());
  }
  auto ribBack = RoutingInformationBase::fromThrift(rib.toThrift());
  EXPECT_EQ(ribBack->toThrift(), rib.toThrift());
}
