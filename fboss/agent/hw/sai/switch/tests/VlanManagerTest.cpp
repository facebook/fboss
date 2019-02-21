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

#include <gtest/gtest.h>

using namespace facebook::fboss;

/*
 * In these tests, we will assume 4 ports with one lane each, with IDs
 */

class VlanManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    ManagerTestBase::SetUp();
    setupPorts();
  }

  void setupPorts() {
    for (int i = 0; i < 4; ++i) {
      addPort(i, true);
    }
  }

  /*
   * This assumes one lane ports where the HwLaneList attribute
   * has the port id in "expectedPorts"
   */
  void checkVlanMembers(
      sai_object_id_t saiVlanId,
      const std::unordered_set<uint32_t>& expectedPorts) const {
    std::unordered_set<uint32_t> observedPorts;
    auto& vlanApi = saiApiTable->vlanApi();
    auto& bridgeApi = saiApiTable->bridgeApi();
    auto& portApi = saiApiTable->portApi();
    std::vector<sai_object_id_t> members;
    members.resize(expectedPorts.size());
    auto gotMembers = vlanApi.getAttribute(
        VlanApiParameters::Attributes::MemberList(members), saiVlanId);
    for (const auto& member : gotMembers) {
      auto bridgePortId = vlanApi.getMemberAttribute(
          VlanApiParameters::MemberAttributes::BridgePortId(), member);
      auto portId = bridgeApi.getMemberAttribute(
          BridgeApiParameters::MemberAttributes::PortId(), bridgePortId);
      std::vector<uint32_t> lanes;
      lanes.resize(1);
      auto lanesGot = portApi.getAttribute(
          PortApiParameters::Attributes::HwLaneList(lanes), portId);
      observedPorts.insert(lanesGot[0]);
    }
    EXPECT_EQ(observedPorts, expectedPorts);
  }
};

TEST_F(VlanManagerTest, addVlan) {
  auto saiId = addVlan(42, {});
  auto swId = saiApiTable->vlanApi().getAttribute(
      VlanApiParameters::Attributes::VlanId(), saiId);
  EXPECT_EQ(swId, 42);
}

TEST_F(VlanManagerTest, addTwoVlans) {
  auto saiId = addVlan(42, {});
  auto swId = saiApiTable->vlanApi().getAttribute(
      VlanApiParameters::Attributes::VlanId(), saiId);
  EXPECT_EQ(swId, 42);
  saiId = addVlan(43, {});
  swId = saiApiTable->vlanApi().getAttribute(
      VlanApiParameters::Attributes::VlanId(), saiId);
  EXPECT_EQ(swId, 43);
}

TEST_F(VlanManagerTest, addDupVlan) {
  auto saiId = addVlan(42, {});
  EXPECT_THROW(addVlan(42, {}), FbossError);
}

TEST_F(VlanManagerTest, getVlan) {
  auto saiId = addVlan(42, {});
  auto swId = VlanID(42);
  auto& vlanManager = saiManagerTable->vlanManager();
  SaiVlan* vlan = vlanManager.getVlan(swId);
  EXPECT_TRUE(vlan);
}

TEST_F(VlanManagerTest, getNonexistentVlan) {
  auto saiId = addVlan(42, {});
  auto swId = VlanID(43);
  auto& vlanManager = saiManagerTable->vlanManager();
  SaiVlan* vlan = vlanManager.getVlan(swId);
  EXPECT_FALSE(vlan);
}

TEST_F(VlanManagerTest, removeVlan) {
  auto saiId = addVlan(42, {});
  auto swId = VlanID(42);
  auto& vlanManager = saiManagerTable->vlanManager();
  vlanManager.removeVlan(swId);
  SaiVlan* vlan = vlanManager.getVlan(swId);
  EXPECT_FALSE(vlan);
}

TEST_F(VlanManagerTest, removeNonexistentVlan) {
  auto saiId = addVlan(42, {});
  auto swId = VlanID(43);
  auto& vlanManager = saiManagerTable->vlanManager();
  EXPECT_THROW(vlanManager.removeVlan(swId), FbossError);
}

TEST_F(VlanManagerTest, addVlanMember) {
  auto saiId = addVlan(42, {0, 2, 3});
  checkVlanMembers(saiId, {0, 2, 3});
  auto swId = VlanID(42);
  auto& vlanManager = saiManagerTable->vlanManager();
  SaiVlan* vlan = vlanManager.getVlan(swId);
  EXPECT_TRUE(vlan);
  vlan->addMember(PortID(1));
  checkVlanMembers(saiId, {0, 1, 2, 3});
}

TEST_F(VlanManagerTest, removeVlanMember) {
  auto saiId = addVlan(42, {0, 1, 2, 3});
  checkVlanMembers(saiId, {0, 1, 2, 3});
  auto swId = VlanID(42);
  auto& vlanManager = saiManagerTable->vlanManager();
  SaiVlan* vlan = vlanManager.getVlan(swId);
  EXPECT_TRUE(vlan);
  vlan->removeMember(PortID(1));
  checkVlanMembers(saiId, {0, 2, 3});
}

TEST_F(VlanManagerTest, changeVlanAddMembers) {
  auto oldVlan = makeVlan(42, {0, 2});
  auto newVlan = makeVlan(42, {0, 1, 2, 3});
  auto saiId = saiManagerTable->vlanManager().addVlan(oldVlan);
  checkVlanMembers(saiId, {0, 2});
  saiManagerTable->vlanManager().changeVlan(oldVlan, newVlan);
  checkVlanMembers(saiId, {0, 1, 2, 3});
}

TEST_F(VlanManagerTest, changeVlanRemoveMembers) {
  auto oldVlan = makeVlan(42, {0, 1, 2, 3});
  auto newVlan = makeVlan(42, {0, 1, 3});
  auto saiId = saiManagerTable->vlanManager().addVlan(oldVlan);
  checkVlanMembers(saiId, {0, 1, 2, 3});
  saiManagerTable->vlanManager().changeVlan(oldVlan, newVlan);
  checkVlanMembers(saiId, {0, 1, 3});
}

TEST_F(VlanManagerTest, changeVlanAddRemoveMembers) {
  auto oldVlan = makeVlan(42, {0, 1, 3});
  auto newVlan = makeVlan(42, {1, 2, 3});
  auto saiId = saiManagerTable->vlanManager().addVlan(oldVlan);
  checkVlanMembers(saiId, {0, 1, 3});
  saiManagerTable->vlanManager().changeVlan(oldVlan, newVlan);
  checkVlanMembers(saiId, {1, 2, 3});
}

TEST_F(VlanManagerTest, removeVlanWithMembers) {
  auto saiId = addVlan(42, {0, 1, 2, 3});
  checkVlanMembers(saiId, {0, 1, 2, 3});
  auto swId = VlanID(42);
  auto& vlanManager = saiManagerTable->vlanManager();
  vlanManager.removeVlan(swId);
  EXPECT_FALSE(vlanManager.getVlan(swId));
}

TEST_F(VlanManagerTest, addBadVlanMember) {
  EXPECT_THROW(addVlan(42, {0, 1, 2, 4}), FbossError);
}

TEST_F(VlanManagerTest, removeNonexistentVlanMember) {
  auto saiId = addVlan(42, {0, 1, 2, 3});
  auto swId = VlanID(42);
  auto& vlanManager = saiManagerTable->vlanManager();
  SaiVlan* vlan = vlanManager.getVlan(swId);
  EXPECT_TRUE(vlan);
  EXPECT_THROW(vlan->removeMember(PortID(5)), FbossError);
}
