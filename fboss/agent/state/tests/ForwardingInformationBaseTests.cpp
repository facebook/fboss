/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <gtest/gtest.h>
#include <memory>

namespace {
template <typename AddressT>
std::shared_ptr<facebook::fboss::Route<AddressT>> createRouteFromPrefix(
    facebook::fboss::RoutePrefix<AddressT> prefix) {
  facebook::fboss::RouteFields<AddressT> fields(prefix);

  return std::make_shared<facebook::fboss::Route<AddressT>>(fields);
}

} // namespace

namespace facebook::fboss {

TEST(ForwardingInformationBaseV4, IPv4DefaultPrefixComparesSmallest) {
  ForwardingInformationBaseV4 oldFib;
  ForwardingInformationBaseV4 newFib;

  RoutePrefixV4 curPrefix;

  // The following loop iterates over 0.0.0.0/0, 0.0.0.0/1, ..., 0.0.0.0/32
  curPrefix.network = folly::IPAddressV4("0.0.0.0");
  for (uint8_t len = 0; len <= 32; ++len) {
    curPrefix.mask = len;

    newFib.addNode(createRouteFromPrefix(curPrefix));
  }

  // The following loops iterate over the following set of prefixes:
  // {2^i | i in {1, ..., 32} x {1, ..., 32}
  for (uint8_t len = 1; len <= 32; ++len) {
    curPrefix.mask = len;

    for (uint32_t b = (static_cast<uint32_t>(1) << 31); b > 0; b >>= 1) {
      curPrefix.network = folly::IPAddressV4::fromLongHBO(b);
      newFib.addNode(createRouteFromPrefix(curPrefix));
    }
  }

  NodeMapDelta<ForwardingInformationBaseV4> delta(&oldFib, &newFib);
  std::shared_ptr<RouteV4> firstRouteObserved;

  DeltaFunctions::forEachAdded(delta, [&](std::shared_ptr<RouteV4> newRoute) {
    firstRouteObserved = newRoute;
    return LoopAction::BREAK;
  });

  EXPECT_EQ(
      firstRouteObserved->prefix().network, folly::IPAddressV4("0.0.0.0"));
  EXPECT_EQ(firstRouteObserved->prefix().mask, 0);
}

TEST(ForwardingInformationBaseV6, IPv6DefaultPrefixComparesSmallest) {
  ForwardingInformationBaseV6 oldFib;
  ForwardingInformationBaseV6 newFib;

  RoutePrefixV6 curPrefix;

  // The following loop iterates over ::/0, ::/1, ..., ::/128
  curPrefix.network = folly::IPAddressV6("::");
  for (uint8_t len = 0; len <= 128; ++len) {
    curPrefix.mask = len;

    newFib.addNode(createRouteFromPrefix(curPrefix));
  }

  std::array<uint8_t, 16> bytes;
  bytes.fill(0);
  auto byteRange = folly::range(bytes.begin(), bytes.end());

  // The following loops iterate over the the following prefixes:
  // {2^i | i in {1, ..., 128} x {1, ..., 128}
  for (uint8_t len = 1; len <= 128; ++len) {
    curPrefix.mask = len;

    for (uint8_t byteIdx = 0; byteIdx < 16; ++byteIdx) {
      auto curByte = &bytes[byteIdx];

      for (uint8_t bitIdx = 0; bitIdx < 8; ++bitIdx) {
        *curByte = 1 << bitIdx;

        curPrefix.network = folly::IPAddressV6::fromBinary(byteRange);
        newFib.addNode(createRouteFromPrefix(curPrefix));
      }

      *curByte = 0;
    }
  }

  NodeMapDelta<ForwardingInformationBaseV6> delta(&oldFib, &newFib);
  std::shared_ptr<RouteV6> firstRouteObserved;

  DeltaFunctions::forEachAdded(delta, [&](std::shared_ptr<RouteV6> newRoute) {
    firstRouteObserved = newRoute;
    return LoopAction::BREAK;
  });

  EXPECT_EQ(firstRouteObserved->prefix().network, folly::IPAddressV6("::"));
  EXPECT_EQ(firstRouteObserved->prefix().mask, 0);
}

} // namespace facebook::fboss
