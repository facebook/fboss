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
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

/*
 * In these tests, we will assume 4 ports with one lane each, with IDs
 */

class VlanManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    intf1 = testInterfaces[1];
  }
  /*
   * This assumes one lane ports where the HwLaneList attribute
   * has the port id in "expectedPorts"
   */
  void checkVlanMembers(
      VlanSaiId saiVlanId,
      const std::unordered_set<uint32_t>& expectedPorts) const {
    XLOG(INFO) << "check vlan members " << saiVlanId;
    std::unordered_set<uint32_t> observedPorts;
    auto& vlanApi = saiApiTable->vlanApi();
    auto& bridgeApi = saiApiTable->bridgeApi();
    auto& portApi = saiApiTable->portApi();
    auto gotMembers = vlanApi.getAttribute(
        saiVlanId, SaiVlanTraits::Attributes::MemberList{});
    for (const auto& member : gotMembers) {
      BridgePortSaiId bridgePortId{vlanApi.getAttribute(
          VlanMemberSaiId{member},
          SaiVlanMemberTraits::Attributes::BridgePortId{})};
      PortSaiId portId{bridgeApi.getAttribute(
          bridgePortId, SaiBridgePortTraits::Attributes::PortId{})};
      auto lanesGot =
          portApi.getAttribute(portId, SaiPortTraits::Attributes::HwLaneList{});
      observedPorts.insert(lanesGot[0]);
      XLOG(INFO) << "Check Vlan Member: " << member
                 << "; bpid: " << bridgePortId << "; portId" << portId << "; "
                 << lanesGot[0];
    }
    EXPECT_EQ(observedPorts, expectedPorts);
  }
  TestInterface intf0;
  TestInterface intf1;
};

TEST_F(VlanManagerTest, addVlan) {
  std::shared_ptr<Vlan> swVlan = makeVlan(intf0);
  VlanSaiId saiId = saiManagerTable->vlanManager().addVlan(swVlan);
  auto swId = saiApiTable->vlanApi().getAttribute(
      saiId, SaiVlanTraits::Attributes::VlanId());
  EXPECT_EQ(swId, 0);
}

TEST_F(VlanManagerTest, addTwoVlans) {
  std::shared_ptr<Vlan> swVlan = makeVlan(intf0);
  VlanSaiId saiId = saiManagerTable->vlanManager().addVlan(swVlan);
  auto swId = saiApiTable->vlanApi().getAttribute(
      saiId, SaiVlanTraits::Attributes::VlanId());
  EXPECT_EQ(swId, 0);
  std::shared_ptr<Vlan> swVlan2 = makeVlan(intf1);
  VlanSaiId saiId2 = saiManagerTable->vlanManager().addVlan(swVlan2);
  auto swId2 = saiApiTable->vlanApi().getAttribute(
      saiId2, SaiVlanTraits::Attributes::VlanId());
  EXPECT_EQ(swId2, 1);
}

TEST_F(VlanManagerTest, addDupVlan) {
  std::shared_ptr<Vlan> swVlan = makeVlan(intf0);
  saiManagerTable->vlanManager().addVlan(swVlan);
  EXPECT_THROW(saiManagerTable->vlanManager().addVlan(swVlan), FbossError);
}

TEST_F(VlanManagerTest, getVlan) {
  std::shared_ptr<Vlan> swVlan = makeVlan(intf0);
  saiManagerTable->vlanManager().addVlan(swVlan);
  auto swId = VlanID(0);
  auto& vlanManager = saiManagerTable->vlanManager();
  auto handle = vlanManager.getVlanHandle(swId);
  EXPECT_TRUE(handle);
  EXPECT_TRUE(handle->vlan);
  // TODO: test vlan is actually present
}

TEST_F(VlanManagerTest, getNonexistentVlan) {
  std::shared_ptr<Vlan> swVlan = makeVlan(intf0);
  saiManagerTable->vlanManager().addVlan(swVlan);
  auto swId = VlanID(10);
  auto& vlanManager = saiManagerTable->vlanManager();
  auto handle = vlanManager.getVlanHandle(swId);
  EXPECT_FALSE(handle);
}

TEST_F(VlanManagerTest, removeVlan) {
  std::shared_ptr<Vlan> swVlan = makeVlan(intf0);
  auto& vlanManager = saiManagerTable->vlanManager();
  vlanManager.addVlan(swVlan);
  auto swId = VlanID(0);
  auto handle = vlanManager.getVlanHandle(swId);
  EXPECT_TRUE(handle);
  vlanManager.removeVlan(swVlan);
  handle = vlanManager.getVlanHandle(swId);
  EXPECT_FALSE(handle);
  // TODO: test vlan is gone
}

TEST_F(VlanManagerTest, removeNonexistentVlan) {
  std::shared_ptr<Vlan> swVlan = makeVlan(intf0);
  auto& vlanManager = saiManagerTable->vlanManager();
  vlanManager.addVlan(swVlan);
  TestInterface intf10;
  intf10.id = 10;
  auto vlan10 = makeVlan(intf10);
  EXPECT_THROW(vlanManager.removeVlan(vlan10), FbossError);
}

TEST_F(VlanManagerTest, changeVlanAddMembers) {
  TestInterface intf{0, 2};
  std::shared_ptr<Vlan> oldVlan = makeVlan(intf);
  auto saiId = saiManagerTable->vlanManager().addVlan(oldVlan);
  checkVlanMembers(saiId, {0, 1});
  std::shared_ptr<Vlan> newVlan = makeVlan(intf0);
  saiManagerTable->vlanManager().changeVlan(oldVlan, newVlan);
  checkVlanMembers(saiId, {0, 1, 2, 3});
}

TEST_F(VlanManagerTest, changeVlanRemoveMembers) {
  TestInterface intf{0, 3};
  std::shared_ptr<Vlan> oldVlan = makeVlan(intf0);
  std::shared_ptr<Vlan> newVlan = makeVlan(intf);
  auto saiId = saiManagerTable->vlanManager().addVlan(oldVlan);
  checkVlanMembers(saiId, {0, 1, 2, 3});
  saiManagerTable->vlanManager().changeVlan(oldVlan, newVlan);
  checkVlanMembers(saiId, {0, 1, 2});
}

TEST_F(VlanManagerTest, changeVlanAddRemoveMembers) {
  TestInterface intf{0, 3};
  intf.remoteHosts[2].port.id = 3;
  std::shared_ptr<Vlan> oldVlan = makeVlan(intf);
  intf.remoteHosts[0].port.id = 1;
  intf.remoteHosts[1].port.id = 2;
  intf.remoteHosts[2].port.id = 3;
  std::shared_ptr<Vlan> newVlan = makeVlan(intf);
  auto saiId = saiManagerTable->vlanManager().addVlan(oldVlan);
  checkVlanMembers(saiId, {0, 1, 3});
  saiManagerTable->vlanManager().changeVlan(oldVlan, newVlan);
  checkVlanMembers(saiId, {1, 2, 3});
}

TEST_F(VlanManagerTest, removeVlanWithMembers) {
  std::shared_ptr<Vlan> swVlan = makeVlan(intf0);
  auto saiId = saiManagerTable->vlanManager().addVlan(swVlan);
  checkVlanMembers(saiId, {0, 1, 2, 3});
  auto& vlanManager = saiManagerTable->vlanManager();
  vlanManager.removeVlan(swVlan);
  auto swId = VlanID(0);
  EXPECT_FALSE(vlanManager.getVlanHandle(swId));
}

TEST_F(VlanManagerTest, addBadVlanMember) {
  TestInterface intf{0, 5};
  std::shared_ptr<Vlan> swVlan = makeVlan(intf);
  EXPECT_THROW(saiManagerTable->vlanManager().addVlan(swVlan), FbossError);
}
