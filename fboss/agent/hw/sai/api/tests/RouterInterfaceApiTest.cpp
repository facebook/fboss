/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class RouterInterfaceApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    routerInterfaceApi = std::make_unique<RouterInterfaceApi>();
  }
  sai_object_id_t createRouterInterface(
      sai_object_id_t vr = 42,
      sai_object_id_t vlan = 43) {
    RouterInterfaceTypes::AttributeType typeAttribute =
        RouterInterfaceTypes::Attributes::Type(SAI_ROUTER_INTERFACE_TYPE_VLAN);
    RouterInterfaceTypes::AttributeType virtualRouterIdAttribute =
        RouterInterfaceTypes::Attributes::VirtualRouterId(vr);
    RouterInterfaceTypes::AttributeType vlanIdAttribute =
        RouterInterfaceTypes::Attributes::VlanId(vlan);
    auto rifId = routerInterfaceApi->create(
        {typeAttribute, virtualRouterIdAttribute, vlanIdAttribute}, 0);
    EXPECT_EQ(rifId, fs->rim.get(rifId).id);
    EXPECT_EQ(vr, fs->rim.get(rifId).virtualRouterId);
    EXPECT_EQ(vlan, fs->rim.get(rifId).vlanId);
    return rifId;
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<RouterInterfaceApi> routerInterfaceApi;
  sai_object_id_t switchId;
};

TEST_F(RouterInterfaceApiTest, create) {
  createRouterInterface();
}

TEST_F(RouterInterfaceApiTest, badCreate) {
  RouterInterfaceTypes::AttributeType typeAttribute =
      RouterInterfaceTypes::Attributes::Type(SAI_ROUTER_INTERFACE_TYPE_VLAN);
  RouterInterfaceTypes::AttributeType virtualRouterIdAttribute =
      RouterInterfaceTypes::Attributes::VirtualRouterId(0);
  RouterInterfaceTypes::AttributeType vlanIdAttribute =
      RouterInterfaceTypes::Attributes::VlanId(0);
  EXPECT_THROW(routerInterfaceApi->create({}, 0), SaiApiError);
  EXPECT_THROW(routerInterfaceApi->create({vlanIdAttribute}, 0), SaiApiError);
  EXPECT_THROW(
      routerInterfaceApi->create({virtualRouterIdAttribute}, 0), SaiApiError);
  EXPECT_THROW(
      routerInterfaceApi->create(
          {virtualRouterIdAttribute, vlanIdAttribute}, 0),
      SaiApiError);
  EXPECT_THROW(routerInterfaceApi->create({typeAttribute}, 0), SaiApiError);
  EXPECT_THROW(
      routerInterfaceApi->create({typeAttribute, vlanIdAttribute}, 0),
      SaiApiError);
  EXPECT_THROW(
      routerInterfaceApi->create({typeAttribute, virtualRouterIdAttribute}, 0),
      SaiApiError);
  routerInterfaceApi->create(
      {typeAttribute, virtualRouterIdAttribute, vlanIdAttribute}, 0);
}

TEST_F(RouterInterfaceApiTest, setSrcMac) {
  auto rifId = createRouterInterface();
  folly::MacAddress mac("42:42:42:42:42:42");
  RouterInterfaceTypes::Attributes::SrcMac ma(mac);
  RouterInterfaceTypes::Attributes::SrcMac ma2;
  EXPECT_NE(mac, routerInterfaceApi->getAttribute(ma2, rifId));
  routerInterfaceApi->setAttribute(ma, rifId);
  EXPECT_EQ(mac, routerInterfaceApi->getAttribute(ma2, rifId));
}

TEST_F(RouterInterfaceApiTest, setVrId) {
  auto rifId = createRouterInterface();
  RouterInterfaceTypes::Attributes::VirtualRouterId virtualRouterIdAttribute(
      10);
  RouterInterfaceTypes::Attributes::VirtualRouterId virtualRouterIdAttribute2;
  EXPECT_EQ(
      42, routerInterfaceApi->getAttribute(virtualRouterIdAttribute2, rifId));
  EXPECT_EQ(
      SAI_STATUS_INVALID_PARAMETER,
      routerInterfaceApi->setAttribute(virtualRouterIdAttribute, rifId));
  EXPECT_EQ(
      42, routerInterfaceApi->getAttribute(virtualRouterIdAttribute2, rifId));
}

TEST_F(RouterInterfaceApiTest, setVlanId) {
  auto rifId = createRouterInterface();
  RouterInterfaceTypes::Attributes::VlanId vlanIdAttribute(10);
  RouterInterfaceTypes::Attributes::VlanId vlanIdAttribute2;
  EXPECT_EQ(43, routerInterfaceApi->getAttribute(vlanIdAttribute2, rifId));
  EXPECT_EQ(
      SAI_STATUS_INVALID_PARAMETER,
      routerInterfaceApi->setAttribute(vlanIdAttribute, rifId));
  EXPECT_EQ(43, routerInterfaceApi->getAttribute(vlanIdAttribute2, rifId));
}
