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
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <gtest/gtest.h>
#include <memory>

namespace {
template <typename AddressT>
std::shared_ptr<facebook::fboss::Route<AddressT>> createRouteFromPrefix(
    facebook::fboss::RoutePrefix<AddressT> prefix) {
  facebook::fboss::RouteFields<AddressT> fields(prefix);

  return std::make_shared<facebook::fboss::Route<AddressT>>(fields.toThrift());
}

std::shared_ptr<facebook::fboss::ForwardingInformationBaseV4> getFibV4() {
  facebook::fboss::ForwardingInformationBaseV4 fibV4;

  facebook::fboss::RoutePrefixV4 curPrefix;

  // The following loop iterates over 0.0.0.0/0, 0.0.0.0/1, ..., 0.0.0.0/32
  auto network = folly::IPAddressV4("0.0.0.0");
  for (uint8_t len = 0; len <= 32; ++len) {
    fibV4.addNode(
        createRouteFromPrefix(facebook::fboss::RoutePrefixV4(network, len)));
  }

  // The following loops iterate over the following set of prefixes:
  // {2^i | i in {1, ..., 32} x {1, ..., 32}
  for (uint8_t len = 1; len <= 32; ++len) {
    for (uint32_t b = (static_cast<uint32_t>(1) << 31); b > 0; b >>= 1) {
      auto ip = folly::IPAddressV4::fromLongHBO(b);
      fibV4.addNode(
          createRouteFromPrefix(facebook::fboss::RoutePrefixV4(ip, len)));
    }
  }
  return fibV4.clone();
}

std::shared_ptr<facebook::fboss::ForwardingInformationBaseV6> getFibV6() {
  facebook::fboss::ForwardingInformationBaseV6 fibV6;
  facebook::fboss::RoutePrefixV6 curPrefix;

  // The following loop iterates over ::/0, ::/1, ..., ::/128
  auto ip = folly::IPAddressV6("::");
  for (uint8_t len = 0; len <= 128; ++len) {
    fibV6.addNode(
        createRouteFromPrefix(facebook::fboss::RoutePrefixV6(ip, len)));
  }

  std::array<uint8_t, 16> bytes;
  bytes.fill(0);
  auto byteRange = folly::range(bytes.begin(), bytes.end());

  // The following loops iterate over the the following prefixes:
  // {2^i | i in {1, ..., 128} x {1, ..., 128}
  for (uint8_t len = 1; len <= 128; ++len) {
    for (uint8_t byteIdx = 0; byteIdx < 16; ++byteIdx) {
      auto curByte = &bytes[byteIdx];

      for (uint8_t bitIdx = 0; bitIdx < 8; ++bitIdx) {
        *curByte = 1 << bitIdx;

        auto ipV6 = folly::IPAddressV6::fromBinary(byteRange);
        fibV6.addNode(
            createRouteFromPrefix(facebook::fboss::RoutePrefixV6(ipV6, len)));
      }

      *curByte = 0;
    }
  }
  return fibV6.clone();
}

} // namespace

namespace facebook::fboss {

TEST(ForwardingInformationBaseV4, IPv4DefaultPrefixComparesSmallest) {
  ForwardingInformationBaseV4 oldFib;
  auto newFib = getFibV4();

  ThriftMapDelta<ForwardingInformationBaseV4> delta(&oldFib, newFib.get());
  std::shared_ptr<RouteV4> firstRouteObserved;

  DeltaFunctions::forEachAdded(delta, [&](std::shared_ptr<RouteV4> newRoute) {
    firstRouteObserved = newRoute;
    return LoopAction::BREAK;
  });

  validateThriftMapMapSerialization(*newFib);
  EXPECT_EQ(
      firstRouteObserved->prefix().network(), folly::IPAddressV4("0.0.0.0"));
  EXPECT_EQ(firstRouteObserved->prefix().mask(), 0);
}

TEST(ForwardingInformationBaseV6, IPv6DefaultPrefixFound) {
  ForwardingInformationBaseV6 oldFib;
  auto newFib = getFibV6();

  validateThriftMapMapSerialization(*newFib);

  ThriftMapDelta<ForwardingInformationBaseV6> delta(&oldFib, newFib.get());
  std::shared_ptr<RouteV6> defaultRouteObserved;

  RoutePrefixV6 defaultPrefixV6{folly::IPAddressV6("::"), 0};

  DeltaFunctions::forEachAdded(delta, [&](std::shared_ptr<RouteV6> newRoute) {
    if (newRoute->getID() == defaultPrefixV6.str()) {
      defaultRouteObserved = newRoute;
      return LoopAction::BREAK;
    }
    return LoopAction::CONTINUE;
  });

  EXPECT_NE(defaultRouteObserved, nullptr);
}

TEST(ForwardingInformationBaseContainer, Thrifty) {
  auto fibV4 = getFibV4();
  auto fibV6 = getFibV4();
  ForwardingInformationBaseContainer container(RouterID(0));
  container.setFib(fibV4);
  container.setFib(fibV6);
  validateNodeSerialization(container);

  std::shared_ptr<ForwardingInformationBaseMap> fibs =
      std::make_shared<ForwardingInformationBaseMap>();
  fibs->addNode(container.clone());
  validateThriftMapMapSerialization(*fibs);
}

} // namespace facebook::fboss
