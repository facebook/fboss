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
#include "fboss/agent/hw/sai/store/SaiObject.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class RouteStoreTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    saiApiTable = SaiApiTable::getInstance();
    saiApiTable->queryApis();
  }
  std::shared_ptr<FakeSai> fs;
  std::shared_ptr<SaiApiTable> saiApiTable;
};

TEST_F(RouteStoreTest, routeLoadCtor) {
  auto& routeApi = saiApiTable->routeApi();
  folly::IPAddress ip4{"10.10.10.1"};
  folly::CIDRNetwork dest(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::Attributes::PacketAction packetActionAttribute{
      SAI_PACKET_ACTION_FORWARD};
  SaiRouteTraits::Attributes::NextHopId nextHopIdAttribute(5);
  routeApi.create2<SaiRouteTraits>(
      r, {packetActionAttribute, nextHopIdAttribute});

  SaiObject<SaiRouteTraits> obj(r);
  EXPECT_EQ(obj.adapterKey(), r);
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, obj.attributes()),
      SAI_PACKET_ACTION_FORWARD);
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, obj.attributes()), 5);
}

TEST_F(RouteStoreTest, routeCreateCtor) {
  folly::IPAddress ip4{"10.10.10.1"};
  folly::CIDRNetwork dest(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::CreateAttributes c{SAI_PACKET_ACTION_FORWARD, 5};
  SaiObject<SaiRouteTraits> obj(r, c, 0);
  EXPECT_EQ(obj.adapterKey(), r);
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, obj.attributes()),
      SAI_PACKET_ACTION_FORWARD);
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, obj.attributes()), 5);
}

TEST_F(RouteStoreTest, routeSetToPunt) {
  folly::IPAddress ip4{"10.10.10.1"};
  folly::CIDRNetwork dest(ip4, 24);
  SaiRouteTraits::RouteEntry r(0, 0, dest);
  SaiRouteTraits::CreateAttributes c{SAI_PACKET_ACTION_FORWARD, 5};
  SaiObject<SaiRouteTraits> obj(r, c, 0);
  EXPECT_EQ(obj.adapterKey(), r);
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, obj.attributes()),
      SAI_PACKET_ACTION_FORWARD);
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, obj.attributes()), 5);

  SaiRouteTraits::CreateAttributes newAttrs{SAI_PACKET_ACTION_TRAP,
                                            std::nullopt};
  obj.setAttributes(newAttrs);
  EXPECT_EQ(
      GET_ATTR(Route, PacketAction, obj.attributes()), SAI_PACKET_ACTION_TRAP);
  /*
   * TODO(borisb): check for default value, once it is supported
  EXPECT_EQ(GET_OPT_ATTR(Route, NextHopId, obj.attributes()), 5);
  */
}
