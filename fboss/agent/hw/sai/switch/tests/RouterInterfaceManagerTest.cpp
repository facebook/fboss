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
    ManagerTestBase::SetUp();
    saiVlanId1 = addVlan(vlanId1, {});
    saiVlanId2 = addVlan(vlanId2, {});
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
    EXPECT_EQ(saiVlanIdGot, expectedSaiVlanId);
    EXPECT_EQ(srcMacGot, expectedSrcMac);
  }

  static constexpr uint16_t vlanId1 = 1;
  static constexpr uint16_t vlanId2 = 2;
  sai_object_id_t saiVlanId1;
  sai_object_id_t saiVlanId2;
  folly::MacAddress srcMac1{"AA:BB:CC:DD:EE:11"};
  folly::MacAddress srcMac2{"AA:BB:CC:DD:EE:22"};
};

TEST_F(RouterInterfaceManagerTest, addRouterInterface) {
  auto saiId = addInterface(1, srcMac1);
  checkRouterInterface(saiId, saiVlanId1, srcMac1);
}

TEST_F(RouterInterfaceManagerTest, addTwoRouterInterfaces) {
  auto saiId1 = addInterface(1, srcMac1);
  auto saiId2 = addInterface(2, srcMac2);
  checkRouterInterface(saiId1, saiVlanId1, srcMac1);
  checkRouterInterface(saiId2, saiVlanId2, srcMac2);
}

TEST_F(RouterInterfaceManagerTest, addDupRouterInterface) {
  auto saiId = addInterface(1, srcMac1);
  EXPECT_THROW(addInterface(1, srcMac1), FbossError);
}

TEST_F(RouterInterfaceManagerTest, getRouterInterface) {
  auto saiId = addInterface(1, srcMac1);
  auto routerInterface =
      saiManagerTable->routerInterfaceManager().getRouterInterface(
          InterfaceID(1));
  EXPECT_TRUE(routerInterface);
  EXPECT_EQ(routerInterface->id(), saiId);
}

TEST_F(RouterInterfaceManagerTest, getNonexistentRouterInterface) {
  auto saiId = addInterface(1, srcMac1);
  auto routerInterface =
      saiManagerTable->routerInterfaceManager().getRouterInterface(
          InterfaceID(2));
  EXPECT_FALSE(routerInterface);
}

TEST_F(RouterInterfaceManagerTest, removeRouterInterface) {
  auto saiId = addInterface(1, srcMac1);
  auto& routerInterfaceManager = saiManagerTable->routerInterfaceManager();
  InterfaceID swId(1);
  routerInterfaceManager.removeRouterInterface(swId);
  auto routerInterface =
      saiManagerTable->routerInterfaceManager().getRouterInterface(swId);
  EXPECT_FALSE(routerInterface);
}

TEST_F(RouterInterfaceManagerTest, removeNonexistentRouterInterface) {
  auto saiId = addInterface(1, srcMac1);
  auto& routerInterfaceManager = saiManagerTable->routerInterfaceManager();
  InterfaceID swId(2);
  EXPECT_THROW(routerInterfaceManager.removeRouterInterface(swId), FbossError);
}
