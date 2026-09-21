/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/MacEntry.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

/*
 * Tests for ManagedFdbEntry pub/sub behavior.
 *
 * ManagedFdbEntry subscribes to SaiBridgePortTraits publisher.
 * When the bridge port appears, ManagedFdbEntry::createObject creates
 * the SAI FDB entry. When the bridge port disappears,
 * ManagedFdbEntry::removeObject cleans up the FDB entry.
 */
class ManagedFdbEntryPubSubTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[1];
  }

  void addMacEntry() {
    auto macEntry = std::make_shared<MacEntry>(
        kMac(),
        PortDescriptor(PortID(intf0.remoteHosts[0].port.id)),
        std::optional<cfg::AclLookupClass>(std::nullopt));
    auto newState = programmedState->clone();
    auto newMacTable = newState->getVlans()
                           ->getNode(VlanID(intf0.id))
                           ->getMacTable()
                           ->modify(VlanID(intf0.id), &newState);
    newMacTable->addEntry(macEntry);
    applyNewState(newState);
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

  bool fdbEntryExists() const {
    auto l2Entries = saiManagerTable->fdbManager().getL2Entries();
    for (const auto& entry : l2Entries) {
      if (*entry.mac() == kMac().toString()) {
        return true;
      }
    }
    return false;
  }

  static folly::MacAddress kMac() {
    return folly::MacAddress{"00:11:11:11:11:11"};
  }

  TestInterface intf0;
};

TEST_F(ManagedFdbEntryPubSubTest, fdbEntryCreatedViaBridgePortPubSub) {
  // Bridge port already exists (from PORT setup). Adding a MAC entry
  // creates a ManagedFdbEntry subscriber which immediately gets notified
  // that the bridge port publisher is alive, creating the SAI FDB entry.
  addMacEntry();
  EXPECT_TRUE(fdbEntryExists());
}

TEST_F(ManagedFdbEntryPubSubTest, fdbEntryRemovedOnPortRemoval) {
  addMacEntry();
  EXPECT_TRUE(fdbEntryExists());

  // Remove the port. This destroys the bridge port, which triggers
  // ManagedFdbEntry::removeObject via pub/sub.
  auto portToRemove = intf0.remoteHosts[0].port;
  auto newState = programmedState->clone();
  auto* ports = newState->getPorts()->modify(&newState);
  ports->removeNode(PortID(portToRemove.id));
  applyNewState(newState);

  EXPECT_FALSE(fdbEntryExists());
}

TEST_F(ManagedFdbEntryPubSubTest, fdbEntryRecreatedOnPortReAdd) {
  addMacEntry();
  EXPECT_TRUE(fdbEntryExists());

  // Remove the port
  auto removedHost = intf0.remoteHosts[0];
  auto newState = programmedState->clone();
  auto* ports = newState->getPorts()->modify(&newState);
  ports->removeNode(PortID(removedHost.port.id));
  applyNewState(newState);
  EXPECT_FALSE(fdbEntryExists());

  // Re-add the port. The new bridge port triggers
  // ManagedFdbEntry::createObject via pub/sub.
  auto reAddState = programmedState->clone();
  auto* portsAgain = reAddState->getPorts()->modify(&reAddState);
  auto swPort = makePort(removedHost.port);
  portsAgain->addNode(swPort, scopeResolver().scope(swPort));
  applyNewState(reAddState);

  EXPECT_TRUE(fdbEntryExists());
}
