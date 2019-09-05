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
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;
class NeighborManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    h0 = intf0.remoteHosts[0];
  }

  template <typename NeighborEntryT>
  void checkEntry(
      const NeighborEntryT& neighborEntry,
      const folly::MacAddress& expectedDstMac) {
    auto saiEntry =
        saiManagerTable->neighborManager().saiEntryFromSwEntry(neighborEntry);
    auto gotMac = saiApiTable->neighborApi().getAttribute2(
        saiEntry, SaiNeighborTraits::Attributes::DstMac{});
    EXPECT_EQ(gotMac, expectedDstMac);
    auto saiNeighborHandle =
        saiManagerTable->neighborManager().getNeighborHandle(saiEntry);
    EXPECT_TRUE(saiNeighborHandle);
  }

  template <typename NeighborEntryT>
  void checkMissing(const NeighborEntryT& neighborEntry) {
    auto saiEntry =
        saiManagerTable->neighborManager().saiEntryFromSwEntry(neighborEntry);
    auto saiNeighborHandle =
        saiManagerTable->neighborManager().getNeighborHandle(saiEntry);
    EXPECT_FALSE(saiNeighborHandle);
  }

  TestInterface intf0;
  TestRemoteHost h0;
};

TEST_F(NeighborManagerTest, addResolvedNeighbor) {
  auto arpEntry = makeArpEntry(intf0.id, h0);
  saiManagerTable->neighborManager().addNeighbor(arpEntry);
  checkEntry(arpEntry, h0.mac);
}

TEST_F(NeighborManagerTest, removeResolvedNeighbor) {
  auto arpEntry = makeArpEntry(intf0.id, h0);
  saiManagerTable->neighborManager().addNeighbor(arpEntry);
  checkEntry(arpEntry, h0.mac);
  saiManagerTable->neighborManager().removeNeighbor(arpEntry);
  checkMissing(arpEntry);
}

TEST_F(NeighborManagerTest, addUnresolvedNeighbor) {
  auto pendingEntry = makePendingArpEntry(intf0.id, h0);
  EXPECT_TRUE(pendingEntry->isPending());
  saiManagerTable->neighborManager().addNeighbor(pendingEntry);
  // unresolved entries will not be found by getNeighbor
  checkMissing(pendingEntry);
}

TEST_F(NeighborManagerTest, removeUnresolvedNeighbor) {
  auto pendingEntry = makePendingArpEntry(intf0.id, h0);
  EXPECT_TRUE(pendingEntry->isPending());
  saiManagerTable->neighborManager().addNeighbor(pendingEntry);
  checkMissing(pendingEntry);
  saiManagerTable->neighborManager().removeNeighbor(pendingEntry);
  checkMissing(pendingEntry);
}

TEST_F(NeighborManagerTest, resolveNeighbor) {
  auto pendingEntry = makePendingArpEntry(intf0.id, h0);
  EXPECT_TRUE(pendingEntry->isPending());
  saiManagerTable->neighborManager().addNeighbor(pendingEntry);
  auto arpEntry = makeArpEntry(intf0.id, h0);
  saiManagerTable->neighborManager().changeNeighbor(pendingEntry, arpEntry);
  checkEntry(arpEntry, h0.mac);
}

TEST_F(NeighborManagerTest, unresolveNeighbor) {
  auto arpEntry = makeArpEntry(intf0.id, h0);
  saiManagerTable->neighborManager().addNeighbor(arpEntry);
  checkEntry(arpEntry, h0.mac);
  auto pendingEntry = makePendingArpEntry(intf0.id, h0);
  EXPECT_TRUE(pendingEntry->isPending());
  saiManagerTable->neighborManager().changeNeighbor(arpEntry, pendingEntry);
  checkMissing(pendingEntry);
}

TEST_F(NeighborManagerTest, getNonexistentNeighbor) {
  auto arpEntry = makeArpEntry(intf0.id, h0);
  checkMissing(arpEntry);
}

TEST_F(NeighborManagerTest, removeNonexistentNeighbor) {
  auto arpEntry = makeArpEntry(intf0.id, h0);
  EXPECT_THROW(
      saiManagerTable->neighborManager().removeNeighbor(arpEntry), FbossError);
}

TEST_F(NeighborManagerTest, addDuplicateResolvedNeighbor) {
  auto arpEntry = makeArpEntry(intf0.id, h0);
  saiManagerTable->neighborManager().addNeighbor(arpEntry);
  EXPECT_THROW(
      saiManagerTable->neighborManager().addNeighbor(arpEntry), FbossError);
}

TEST_F(NeighborManagerTest, addDuplicateUnresolvedNeighbor) {
  // TODO (D13604051)
}
