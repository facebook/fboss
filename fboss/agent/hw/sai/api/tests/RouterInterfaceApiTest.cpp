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
  RouterInterfaceSaiId createRouterInterface(
      sai_object_id_t vr = 42,
      sai_object_id_t vlan = 43) {
    SaiRouterInterfaceTraits::Attributes::Type typeAttribute(
        SAI_ROUTER_INTERFACE_TYPE_VLAN);
    SaiRouterInterfaceTraits::Attributes::VirtualRouterId
        virtualRouterIdAttribute(vr);
    SaiRouterInterfaceTraits::Attributes::VlanId vlanIdAttribute(vlan);
    auto rifId = routerInterfaceApi->create<SaiRouterInterfaceTraits>(
        {virtualRouterIdAttribute,
         typeAttribute,
         vlanIdAttribute,
         std::nullopt,
         std::nullopt},
        0);
    EXPECT_EQ(rifId, fs->routeInterfaceManager.get(rifId).id);
    EXPECT_EQ(vr, fs->routeInterfaceManager.get(rifId).virtualRouterId);
    EXPECT_EQ(vlan, fs->routeInterfaceManager.get(rifId).vlanId);
    return rifId;
  }
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<RouterInterfaceApi> routerInterfaceApi;
  sai_object_id_t switchId;
};

TEST_F(RouterInterfaceApiTest, create) {
  createRouterInterface();
}

TEST_F(RouterInterfaceApiTest, setSrcMac) {
  auto rifId = createRouterInterface();
  folly::MacAddress mac("42:42:42:42:42:42");
  SaiRouterInterfaceTraits::Attributes::SrcMac ma(mac);
  SaiRouterInterfaceTraits::Attributes::SrcMac ma2;
  EXPECT_NE(mac, routerInterfaceApi->getAttribute(rifId, ma2));
  routerInterfaceApi->setAttribute(rifId, ma);
  EXPECT_EQ(mac, routerInterfaceApi->getAttribute(rifId, ma2));
}

TEST_F(RouterInterfaceApiTest, setVrId) {
  auto rifId = createRouterInterface();
  SaiRouterInterfaceTraits::Attributes::VirtualRouterId
      virtualRouterIdAttribute(10);
  SaiRouterInterfaceTraits::Attributes::VirtualRouterId
      virtualRouterIdAttribute2;
  EXPECT_EQ(
      42, routerInterfaceApi->getAttribute(rifId, virtualRouterIdAttribute2));
  EXPECT_THROW(
      try {
        routerInterfaceApi->setAttribute(rifId, virtualRouterIdAttribute);
      } catch (const SaiApiError& e) {
        EXPECT_EQ(e.getSaiStatus(), SAI_STATUS_INVALID_PARAMETER);
        throw;
      },
      SaiApiError);
  EXPECT_EQ(
      42, routerInterfaceApi->getAttribute(rifId, virtualRouterIdAttribute2));
}

TEST_F(RouterInterfaceApiTest, setVlanId) {
  auto rifId = createRouterInterface();
  SaiRouterInterfaceTraits::Attributes::VlanId vlanIdAttribute(10);
  SaiRouterInterfaceTraits::Attributes::VlanId vlanIdAttribute2;
  EXPECT_EQ(43, routerInterfaceApi->getAttribute(rifId, vlanIdAttribute2));
  EXPECT_THROW(
      try {
        routerInterfaceApi->setAttribute(rifId, vlanIdAttribute);
      } catch (const SaiApiError& e) {
        EXPECT_EQ(e.getSaiStatus(), SAI_STATUS_INVALID_PARAMETER);
        throw;
      },
      SaiApiError);

  EXPECT_EQ(43, routerInterfaceApi->getAttribute(rifId, vlanIdAttribute2));
}

TEST_F(RouterInterfaceApiTest, setMtu) {
  auto rifId = createRouterInterface();
  sai_uint32_t mtu{9000};
  SaiRouterInterfaceTraits::Attributes::Mtu mtu1{mtu};
  SaiRouterInterfaceTraits::Attributes::Mtu mtu2;
  EXPECT_NE(mtu, routerInterfaceApi->getAttribute(rifId, mtu2));
  EXPECT_EQ(1514 /*default*/, routerInterfaceApi->getAttribute(rifId, mtu2));
  routerInterfaceApi->setAttribute(rifId, mtu1);
  EXPECT_EQ(mtu, routerInterfaceApi->getAttribute(rifId, mtu2));
}

TEST_F(RouterInterfaceApiTest, formatRouterInterfaceAttributes) {
  std::string macStr{"42:42:42:42:42:42"};
  SaiRouterInterfaceTraits::Attributes::SrcMac sm{folly::MacAddress{macStr}};
  EXPECT_EQ(fmt::format("SrcMac: {}", macStr), fmt::format("{}", sm));
  SaiRouterInterfaceTraits::Attributes::Type t{SAI_ROUTER_INTERFACE_TYPE_VLAN};
  EXPECT_EQ("Type: 1", fmt::format("{}", t));
  SaiRouterInterfaceTraits::Attributes::VirtualRouterId vrid{0};
  EXPECT_EQ("VirtualRouterId: 0", fmt::format("{}", vrid));
  SaiRouterInterfaceTraits::Attributes::VlanId vid{42};
  EXPECT_EQ("VlanId: 42", fmt::format("{}", vid));
  SaiRouterInterfaceTraits::Attributes::Mtu m{9000};
  EXPECT_EQ("Mtu: 9000", fmt::format("{}", m));
}
