/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

static constexpr folly::StringPiece str4 = "42.42.12.34";
static constexpr folly::StringPiece str6 =
    "4242:4242:4242:4242:1234:1234:1234:1234";
static constexpr folly::StringPiece strMac = "42:42:42:12:34:56";

class RouteApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    routeApi = std::make_unique<RouteApi>();
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<RouteApi> routeApi;
  folly::IPAddress ip4{str4};
  folly::IPAddress ip6{str6};
  folly::MacAddress dstMac{strMac};
};

TEST_F(RouteApiTest, create2V4Route) {
  folly::CIDRNetwork prefix(ip4, 24);
  RouteApiParameters::RouteEntry r(0, 0, prefix);
  RouteApiParameters::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  RouteApiParameters::Attributes::NextHopId nextHopIdAttribute(5);
  routeApi->create(r, {packetActionAttribute, nextHopIdAttribute});
  EXPECT_EQ(
      routeApi->getAttribute(RouteApiParameters::Attributes::NextHopId(), r),
      5);
}

TEST_F(RouteApiTest, create2V6Route) {
  folly::CIDRNetwork prefix(ip6, 64);
  RouteApiParameters::RouteEntry r(0, 0, prefix);
  RouteApiParameters::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  RouteApiParameters::Attributes::NextHopId nextHopIdAttribute(5);
  routeApi->create(r, {packetActionAttribute, nextHopIdAttribute});
  EXPECT_EQ(
      routeApi->getAttribute(RouteApiParameters::Attributes::NextHopId(), r),
      5);
}

TEST_F(RouteApiTest, setRouteNextHop) {
  folly::CIDRNetwork prefix(ip4, 24);
  RouteApiParameters::RouteEntry r(0, 0, prefix);
  RouteApiParameters::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  RouteApiParameters::Attributes::NextHopId nextHopIdAttribute(5);
  routeApi->create(r, {packetActionAttribute, nextHopIdAttribute});
  RouteApiParameters::Attributes::NextHopId nextHopIdAttribute2(0);
  routeApi->setAttribute(nextHopIdAttribute2, r);
  EXPECT_EQ(
      routeApi->getAttribute(RouteApiParameters::Attributes::NextHopId(), r),
      0);
  RouteApiParameters::Attributes::NextHopId nextHopIdAttribute3(42);
  routeApi->setAttribute(nextHopIdAttribute3, r);
  EXPECT_EQ(
      routeApi->getAttribute(RouteApiParameters::Attributes::NextHopId(), r),
      42);
}
