/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

/*
 * In these tests, we will assume 4 ports with one lane each, with IDs
 */

class RouterInterfaceManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    intf1 = testInterfaces[1];
  }

  void checkRouterInterface(
      RouterInterfaceSaiId saiRouterInterfaceId,
      VlanSaiId expectedSaiVlanId,
      const folly::MacAddress& expectedSrcMac,
      int expectedMtu = 1500) {
    auto saiVlanIdGot = saiApiTable->routerInterfaceApi().getAttribute(
        saiRouterInterfaceId, SaiRouterInterfaceTraits::Attributes::VlanId{});
    auto srcMacGot = saiApiTable->routerInterfaceApi().getAttribute(
        saiRouterInterfaceId, SaiRouterInterfaceTraits::Attributes::SrcMac{});
    auto vlanIdGot = saiApiTable->vlanApi().getAttribute(
        VlanSaiId{saiVlanIdGot}, SaiVlanTraits::Attributes::VlanId{});
    auto mtuGot = saiApiTable->routerInterfaceApi().getAttribute(
        saiRouterInterfaceId, SaiRouterInterfaceTraits::Attributes::Mtu{});
    EXPECT_EQ(VlanSaiId{vlanIdGot}, expectedSaiVlanId);
    EXPECT_EQ(srcMacGot, expectedSrcMac);
    EXPECT_EQ(mtuGot, expectedMtu);
  }

  TestInterface intf0;
  TestInterface intf1;
};

TEST_F(RouterInterfaceManagerTest, addRouterInterface) {
  auto swInterface = makeInterface(intf0);
  auto saiId =
      saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  checkRouterInterface(saiId, VlanSaiId{intf0.id}, intf0.routerMac);
}

TEST_F(RouterInterfaceManagerTest, addTwoRouterInterfaces) {
  auto swInterface0 = makeInterface(intf0);
  auto saiId0 = saiManagerTable->routerInterfaceManager().addRouterInterface(
      swInterface0);
  auto swInterface1 = makeInterface(intf1);
  auto saiId1 = saiManagerTable->routerInterfaceManager().addRouterInterface(
      swInterface1);
  checkRouterInterface(saiId0, VlanSaiId{intf0.id}, intf0.routerMac);
  checkRouterInterface(saiId1, VlanSaiId{intf1.id}, intf1.routerMac);
}

TEST_F(RouterInterfaceManagerTest, addDupRouterInterface) {
  auto swInterface = makeInterface(intf0);
  saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  EXPECT_THROW(
      saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface),
      FbossError);
}

TEST_F(RouterInterfaceManagerTest, changeRouterInterfaceMac) {
  auto swInterface = makeInterface(intf0);
  auto saiId =
      saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  checkRouterInterface(saiId, VlanSaiId{intf0.id}, intf0.routerMac);
  auto newMac = intf1.routerMac;
  CHECK_NE(swInterface->getMac(), newMac);
  auto newInterface = swInterface->clone();
  newInterface->setMac(newMac);
  saiManagerTable->routerInterfaceManager().changeRouterInterface(
      swInterface, newInterface);
  checkRouterInterface(saiId, VlanSaiId{intf0.id}, newMac);
}

TEST_F(RouterInterfaceManagerTest, changeRouterInterfaceMtu) {
  auto swInterface = makeInterface(intf0);
  auto saiId =
      saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  checkRouterInterface(saiId, VlanSaiId{intf0.id}, intf0.routerMac);
  auto newMtu = intf0.mtu + 1000;
  auto newInterface = swInterface->clone();
  newInterface->setMtu(newMtu);
  saiManagerTable->routerInterfaceManager().changeRouterInterface(
      swInterface, newInterface);
  checkRouterInterface(saiId, VlanSaiId{intf0.id}, intf0.routerMac, newMtu);
}

TEST_F(RouterInterfaceManagerTest, getRouterInterface) {
  auto swInterface = makeInterface(intf0);
  auto saiId =
      saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  auto routerInterfaceHandle =
      saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
          InterfaceID(0));
  EXPECT_TRUE(routerInterfaceHandle);
  EXPECT_EQ(routerInterfaceHandle->routerInterface->adapterKey(), saiId);
}

TEST_F(RouterInterfaceManagerTest, getNonexistentRouterInterface) {
  auto swInterface = makeInterface(intf0);
  saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  auto routerInterfaceHandle =
      saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
          InterfaceID(2));
  EXPECT_FALSE(routerInterfaceHandle);
}

TEST_F(RouterInterfaceManagerTest, removeRouterInterface) {
  auto swInterface = makeInterface(intf1);
  saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  auto& routerInterfaceManager = saiManagerTable->routerInterfaceManager();
  InterfaceID swId(1);
  routerInterfaceManager.removeRouterInterface(swId);
  auto routerInterfaceHandle =
      saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(swId);
  EXPECT_FALSE(routerInterfaceHandle);
}

TEST_F(RouterInterfaceManagerTest, removeNonexistentRouterInterface) {
  auto swInterface = makeInterface(intf1);
  saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  auto& routerInterfaceManager = saiManagerTable->routerInterfaceManager();
  InterfaceID swId(2);
  EXPECT_THROW(routerInterfaceManager.removeRouterInterface(swId), FbossError);
}
