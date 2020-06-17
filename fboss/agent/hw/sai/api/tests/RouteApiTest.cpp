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
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
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

TEST_F(RouteApiTest, createV4Route) {
  folly::CIDRNetwork prefix(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  routeApi->create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, std::nullopt});
  EXPECT_EQ(
      routeApi->getAttribute(r, SaiRouteTraits::Attributes::NextHopId()), 5);
}

TEST_F(RouteApiTest, createV6Route) {
  folly::CIDRNetwork prefix(ip6, 64);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  routeApi->create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, std::nullopt});
  EXPECT_EQ(
      routeApi->getAttribute(r, SaiRouteTraits::Attributes::NextHopId()), 5);
}

TEST_F(RouteApiTest, createV4Drop) {
  folly::CIDRNetwork prefix(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_DROP};
  routeApi->create<SaiRouteTraits>(
      r, {packetActionAttribute, std::nullopt, std::nullopt});
  EXPECT_EQ(
      routeApi->getAttribute(r, SaiRouteTraits::Attributes::PacketAction()),
      SAI_PACKET_ACTION_DROP);
}

TEST_F(RouteApiTest, createV6Drop) {
  folly::CIDRNetwork prefix(ip6, 64);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_DROP};
  routeApi->create<SaiRouteTraits>(
      r, {packetActionAttribute, std::nullopt, std::nullopt});
  EXPECT_EQ(
      routeApi->getAttribute(r, SaiRouteTraits::Attributes::PacketAction()),
      SAI_PACKET_ACTION_DROP);
}

TEST_F(RouteApiTest, setRouteNextHop) {
  folly::CIDRNetwork prefix(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  routeApi->create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, std::nullopt});
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute2(0);
  routeApi->setAttribute(r, nextHopIdAttribute2);
  EXPECT_EQ(
      routeApi->getAttribute(r, SaiRouteTraits::Attributes::NextHopId()), 0);
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute3(42);
  routeApi->setAttribute(r, nextHopIdAttribute3);
  EXPECT_EQ(
      routeApi->getAttribute(r, SaiRouteTraits::Attributes::NextHopId()), 42);
}

TEST_F(RouteApiTest, setV4RouteMetadata) {
  folly::CIDRNetwork prefix(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_DROP};
  routeApi->create<SaiRouteTraits>(
      r, {packetActionAttribute, std::nullopt, std::nullopt});
  SaiRouteTraits::Attributes::Metadata metadata(42);
  routeApi->setAttribute(r, metadata);
  EXPECT_EQ(
      routeApi->getAttribute(r, SaiRouteTraits::Attributes::Metadata()), 42);
}

TEST_F(RouteApiTest, setV6RouteMetaData) {
  folly::CIDRNetwork prefix(ip6, 64);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_DROP};
  routeApi->create<SaiRouteTraits>(
      r, {packetActionAttribute, std::nullopt, std::nullopt});
  SaiRouteTraits::Attributes::Metadata metadata(42);
  routeApi->setAttribute(r, metadata);
  EXPECT_EQ(
      routeApi->getAttribute(r, SaiRouteTraits::Attributes::Metadata()), 42);
}

TEST_F(RouteApiTest, setPacketAction) {
  folly::CIDRNetwork prefix(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_DROP};
  routeApi->create<SaiRouteTraits>(
      r, {packetActionAttribute, std::nullopt, std::nullopt});

  packetActionAttribute = SAI_PACKET_ACTION_FORWARD;
  routeApi->setAttribute(r, packetActionAttribute);

  EXPECT_EQ(
      routeApi->getAttribute(r, SaiRouteTraits::Attributes::PacketAction()),
      SAI_PACKET_ACTION_FORWARD);
}

TEST_F(RouteApiTest, routeCount) {
  uint32_t count = getObjectCount<SaiRouteTraits>(0);
  EXPECT_EQ(count, 0);
  folly::CIDRNetwork prefix(ip6, 64);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  routeApi->create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, std::nullopt});
  count = getObjectCount<SaiRouteTraits>(0);
  EXPECT_EQ(count, 1);
}

TEST_F(RouteApiTest, routeKeys) {
  folly::CIDRNetwork prefix(ip6, 64);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  routeApi->create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, std::nullopt});
  auto routeKeys = getObjectKeys<SaiRouteTraits>(0);
  EXPECT_EQ(routeKeys.size(), 1);
  EXPECT_EQ(routeKeys[0], r);
}

TEST_F(RouteApiTest, formatRouteNextHopId) {
  SaiRouteTraits::Attributes::NextHopId nhid{42};
  std::string expected("NextHopId: 42");
  EXPECT_EQ(expected, fmt::format("{}", nhid));
}

TEST(RouteEntryTest, serDeserv6) {
  folly::CIDRNetwork prefix("42::", 64);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  EXPECT_EQ(
      r, SaiRouteTraits::RouteEntry::fromFollyDynamic(r.toFollyDynamic()));
}

TEST(RouteEntryTest, serDeserv4) {
  folly::CIDRNetwork prefix("42.42.42.0", 24);
  SaiRouteTraits::RouteEntry r(0, 0, prefix);
  EXPECT_EQ(
      r, SaiRouteTraits::RouteEntry::fromFollyDynamic(r.toFollyDynamic()));
}
