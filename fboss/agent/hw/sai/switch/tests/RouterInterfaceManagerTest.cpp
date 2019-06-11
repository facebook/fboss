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
      sai_object_id_t saiRouterInterfaceId,
      sai_object_id_t expectedSaiVlanId,
      const folly::MacAddress& expectedSrcMac) {
    auto saiVlanIdGot = saiApiTable->routerInterfaceApi().getAttribute(
        RouterInterfaceApiParameters::Attributes::VlanId(),
        saiRouterInterfaceId);
    auto srcMacGot = saiApiTable->routerInterfaceApi().getAttribute(
        RouterInterfaceApiParameters::Attributes::SrcMac(),
        saiRouterInterfaceId);
    auto vlanIdGot = saiApiTable->vlanApi().getAttribute(
        VlanApiParameters::Attributes::VlanId(), saiVlanIdGot);
    EXPECT_EQ(vlanIdGot, expectedSaiVlanId);
    EXPECT_EQ(srcMacGot, expectedSrcMac);
  }

  TestInterface intf0;
  TestInterface intf1;
};

TEST_F(RouterInterfaceManagerTest, addRouterInterface) {
  auto swInterface = makeInterface(intf0);
  auto saiId =
      saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  checkRouterInterface(saiId, intf0.id, intf0.routerMac);
}

TEST_F(RouterInterfaceManagerTest, addTwoRouterInterfaces) {
  auto swInterface0 = makeInterface(intf0);
  auto saiId0 = saiManagerTable->routerInterfaceManager().addRouterInterface(
      swInterface0);
  auto swInterface1 = makeInterface(intf1);
  auto saiId1 = saiManagerTable->routerInterfaceManager().addRouterInterface(
      swInterface1);
  checkRouterInterface(saiId0, intf0.id, intf0.routerMac);
  checkRouterInterface(saiId1, intf1.id, intf1.routerMac);
}

TEST_F(RouterInterfaceManagerTest, addDupRouterInterface) {
  auto swInterface = makeInterface(intf0);
  saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  EXPECT_THROW(
      saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface),
      FbossError);
}

TEST_F(RouterInterfaceManagerTest, getRouterInterface) {
  auto swInterface = makeInterface(intf0);
  auto saiId =
      saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  auto routerInterface =
      saiManagerTable->routerInterfaceManager().getRouterInterface(
          InterfaceID(0));
  EXPECT_TRUE(routerInterface);
  EXPECT_EQ(routerInterface->id(), saiId);
}

TEST_F(RouterInterfaceManagerTest, getNonexistentRouterInterface) {
  auto swInterface = makeInterface(intf0);
  saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  auto routerInterface =
      saiManagerTable->routerInterfaceManager().getRouterInterface(
          InterfaceID(2));
  EXPECT_FALSE(routerInterface);
}

TEST_F(RouterInterfaceManagerTest, removeRouterInterface) {
  auto swInterface = makeInterface(intf1);
  saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  auto& routerInterfaceManager = saiManagerTable->routerInterfaceManager();
  InterfaceID swId(1);
  routerInterfaceManager.removeRouterInterface(swId);
  auto routerInterface =
      saiManagerTable->routerInterfaceManager().getRouterInterface(swId);
  EXPECT_FALSE(routerInterface);
}

TEST_F(RouterInterfaceManagerTest, removeNonexistentRouterInterface) {
  auto swInterface = makeInterface(intf1);
  saiManagerTable->routerInterfaceManager().addRouterInterface(swInterface);
  auto& routerInterfaceManager = saiManagerTable->routerInterfaceManager();
  InterfaceID swId(2);
  EXPECT_THROW(routerInterfaceManager.removeRouterInterface(swId), FbossError);
}
