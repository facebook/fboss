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
#include "fboss/agent/hw/sai/store/LoggingUtil.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

TEST_F(SaiStoreTest, loadRoute) {
  auto& routeApi = saiApiTable->routeApi();
  folly::IPAddress ip4{"10.10.10.1"};
  folly::CIDRNetwork dest(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  SaiRouteTraits::Attributes::Metadata metadata(42);
  routeApi.create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, metadata});

  std::shared_ptr<SaiStore> s = SaiStore::getInstance();
  s->setSwitchId(0);
  s->reload();
  auto& store = s->get<SaiRouteTraits>();

  auto got = store.get(r);
  EXPECT_EQ(got->adapterKey(), r);
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, got->attributes()), 5);
  EXPECT_EQ(GET_OPT_ATTR(Route, Metadata, got->attributes()), 42);
  store.setObject(r, {packetActionAttribute, 4, 41});
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, got->attributes()), 4);
  EXPECT_EQ(GET_OPT_ATTR(Route, Metadata, got->attributes()), 41);
}

TEST_F(SaiStoreTest, routeLoadCtor) {
  auto& routeApi = saiApiTable->routeApi();
  folly::IPAddress ip4{"10.10.10.1"};
  folly::CIDRNetwork dest(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  SaiRouteTraits::Attributes::Metadata metadata(42);
  routeApi.create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, metadata});

  SaiObject<SaiRouteTraits> obj(r);
  EXPECT_EQ(obj.adapterKey(), r);
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, obj.attributes()),
      SAI_PACKET_ACTION_FORWARD);
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, obj.attributes()), 5);
  EXPECT_EQ(GET_OPT_ATTR(Route, Metadata, obj.attributes()), 42);
}

TEST_F(SaiStoreTest, routeCreateCtor) {
  folly::IPAddress ip4{"10.10.10.1"};
  folly::CIDRNetwork dest(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::CreateAttributes c{SAI_PACKET_ACTION_FORWARD, 5, 42};
  SaiObject<SaiRouteTraits> obj(r, c, 0);
  EXPECT_EQ(obj.adapterKey(), r);
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, obj.attributes()),
      SAI_PACKET_ACTION_FORWARD);
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, obj.attributes()), 5);
  EXPECT_EQ(GET_OPT_ATTR(Route, Metadata, obj.attributes()), 42);
}

TEST_F(SaiStoreTest, routeSetToPunt) {
  folly::IPAddress ip4{"10.10.10.1"};
  folly::CIDRNetwork dest(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::CreateAttributes c{SAI_PACKET_ACTION_FORWARD, 5, 42};
  SaiObject<SaiRouteTraits> obj(r, c, 0);
  EXPECT_EQ(obj.adapterKey(), r);
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, obj.attributes()),
      SAI_PACKET_ACTION_FORWARD);
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, obj.attributes()), 5);
  EXPECT_EQ(GET_OPT_ATTR(Route, Metadata, obj.attributes()), 42);

  SaiRouteTraits::CreateAttributes newAttrs{
      SAI_PACKET_ACTION_TRAP, std::nullopt, 42};
  obj.setAttributes(newAttrs);
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, obj.attributes()), SAI_PACKET_ACTION_TRAP);
  /*
   * TODO(borisb): check for default value, once it is supported
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, obj.attributes()), 5);
  */
}

TEST_F(SaiStoreTest, formatTest) {
  folly::IPAddress ip4{"10.10.10.1"};
  folly::CIDRNetwork dest(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::CreateAttributes c{SAI_PACKET_ACTION_FORWARD, 5, 42};
  SaiObject<SaiRouteTraits> obj(r, c, 0);
  auto expected =
      "RouteEntry(switch: 0, vrf: 0, prefix: 10.10.10.1/24): (PacketAction: 1, NextHopId: 5, Metadata: 42)";
  EXPECT_EQ(expected, fmt::format("{}", obj));
}

TEST_F(SaiStoreTest, serDeserV4Route) {
  auto& routeApi = saiApiTable->routeApi();
  folly::IPAddress ip{"10.10.10.1"};
  folly::CIDRNetwork dest(ip, 24);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  SaiRouteTraits::Attributes::Metadata metadata(42);
  routeApi.create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, metadata});

  verifyAdapterKeySerDeser<SaiRouteTraits>({r});
}

TEST_F(SaiStoreTest, serDeserV6Route) {
  auto& routeApi = saiApiTable->routeApi();
  folly::IPAddress ip{"42::"};
  folly::CIDRNetwork dest(ip, 64);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  SaiRouteTraits::Attributes::Metadata metadata(42);
  routeApi.create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, metadata});

  verifyAdapterKeySerDeser<SaiRouteTraits>({r});
}

TEST_F(SaiStoreTest, toStrV4Route) {
  auto& routeApi = saiApiTable->routeApi();
  folly::IPAddress ip{"10.10.10.1"};
  folly::CIDRNetwork dest(ip, 24);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  SaiRouteTraits::Attributes::Metadata metadata(42);
  routeApi.create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, metadata});

  verifyToStr<SaiRouteTraits>();
}

TEST_F(SaiStoreTest, toStrV6Route) {
  auto& routeApi = saiApiTable->routeApi();
  folly::IPAddress ip{"42::"};
  folly::CIDRNetwork dest(ip, 64);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  SaiRouteTraits::Attributes::Metadata metadata(42);
  routeApi.create<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute, metadata});

  verifyToStr<SaiRouteTraits>();
}
