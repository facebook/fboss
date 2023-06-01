/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/MacEntry.h"
#include "fboss/agent/state/MacTable.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/types.h"

#include <string>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class FdbManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[1];
  }

  void checkFdbEntry(sai_uint32_t metadata = 0) {
    auto vlanHandle = saiManagerTable->vlanManager().getVlanHandle(VlanID(0));
    auto vlanId = vlanHandle->vlan->adapterKey();
    SaiFdbTraits::FdbEntry entry{1, vlanId, kMac()};
    auto portHandle = saiManagerTable->portManager().getPortHandle(
        PortID(intf0.remoteHosts[0].id));
    auto expectedBridgePortId = portHandle->bridgePort->adapterKey();
    auto bridgePortId = saiApiTable->fdbApi().getAttribute(
        entry, SaiFdbTraits::Attributes::BridgePortId{});
    EXPECT_EQ(bridgePortId, expectedBridgePortId);
    EXPECT_EQ(
        saiApiTable->fdbApi().getAttribute(
            entry, SaiFdbTraits::Attributes::Metadata{}),
        metadata);
  }

  void addMacEntry(std::optional<sai_uint32_t> classId = std::nullopt) {
    updateOraAddMacEntry(kMac(), classId, false);
  }

  void updateMacEntry(std::optional<sai_uint32_t> classId = std::nullopt) {
    updateOraAddMacEntry(kMac(), classId, true);
  }

  void removeMacEntry() {
    auto newState = programmedState->clone();
    auto newMacTable = newState->getVlans()
                           ->getNode(VlanID(intf0.id))
                           ->getMacTable()
                           ->modify(VlanID(intf0.id), &newState);
    newMacTable->removeEntry(kMac());
    applyNewState(newState);
  }

  TestInterface intf0;

 private:
  static folly::MacAddress kMac() {
    return folly::MacAddress{"00:11:11:11:11:11"};
  }

  std::shared_ptr<MacEntry> makeMacEntry(
      folly::MacAddress mac,
      std::optional<sai_uint32_t> classId) {
    std::optional<cfg::AclLookupClass> metadata;
    if (classId) {
      metadata = static_cast<cfg::AclLookupClass>(classId.value());
    }
    return std::make_shared<MacEntry>(
        mac,
        PortDescriptor(PortID(intf0.remoteHosts[0].id)),
        std::optional<cfg::AclLookupClass>(metadata));
  }
  void updateOraAddMacEntry(
      folly::MacAddress mac,
      std::optional<sai_uint32_t> classId,
      bool update) {
    auto macEntry = makeMacEntry(mac, classId);
    auto newState = programmedState->clone();
    auto newMacTable = newState->getVlans()
                           ->getNode(VlanID(intf0.id))
                           ->getMacTable()
                           ->modify(VlanID(intf0.id), &newState);
    if (update) {
      newMacTable->updateEntry(
          macEntry->getMac(), macEntry->getPort(), macEntry->getClassID());
    } else {
      newMacTable->addEntry(macEntry);
    }
    applyNewState(newState);
  }
};

TEST_F(FdbManagerTest, addFdbEntry) {
  addMacEntry();
  checkFdbEntry();
}

TEST_F(FdbManagerTest, addFdbEntryWithMetdata) {
  addMacEntry(42);
  checkFdbEntry(42);
}

TEST_F(FdbManagerTest, addRemoveMetadata) {
  addMacEntry();
  checkFdbEntry();
  updateMacEntry(42);
  checkFdbEntry(42);
  updateMacEntry(std::nullopt);
  checkFdbEntry();
}

TEST_F(FdbManagerTest, doubleAddFdbEntry) {
  addMacEntry();
  EXPECT_THROW(addMacEntry(), FbossError);
}

TEST_F(FdbManagerTest, doubleRemoveFdbEntry) {
  addMacEntry();
  removeMacEntry();
  EXPECT_THROW(removeMacEntry(), FbossError);
}
