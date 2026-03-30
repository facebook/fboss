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
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

/*
 * Tests for ManagedVlanRifNeighbor pub/sub behavior.
 *
 * ManagedVlanRifNeighbor subscribes to SaiFdbTraits publisher.
 * When the FDB entry appears, ManagedVlanRifNeighbor::createObject creates
 * the SAI neighbor entry. When the FDB entry disappears,
 * ManagedVlanRifNeighbor::removeObject cleans up the neighbor.
 * On handleLinkDown, the neighbor cascades the notification to its
 * own subscribers (next hops).
 */
class ManagedVlanRifNeighborPubSubTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    h0 = intf0.remoteHosts[0];
  }

  bool neighborExists(const std::shared_ptr<ArpEntry>& arpEntry) const {
    auto saiEntry =
        saiManagerTable->neighborManager().saiEntryFromSwEntry(arpEntry);
    auto handle =
        saiManagerTable->neighborManager().getNeighborHandle(saiEntry);
    return handle && handle->neighbor;
  }

  TestInterface intf0;
  TestRemoteHost h0;
};

TEST_F(ManagedVlanRifNeighborPubSubTest, neighborCreatedViaFdbPubSub) {
  // resolveArp creates both the neighbor and FDB entry. The neighbor
  // subscribes to the FDB publisher. Since the FDB entry is created
  // first (in resolveArp), the neighbor's createObject is called
  // immediately via pub/sub.
  auto arpEntry = resolveArp(intf0.id, h0);
  EXPECT_TRUE(neighborExists(arpEntry));
}

TEST_F(ManagedVlanRifNeighborPubSubTest, neighborRemovedOnFdbRemoval) {
  auto arpEntry = resolveArp(intf0.id, h0);
  EXPECT_TRUE(neighborExists(arpEntry));

  // Remove the FDB entry directly. This triggers
  // ManagedVlanRifNeighbor::removeObject via pub/sub,
  // cleaning up the SAI neighbor.
  saiManagerTable->fdbManager().removeFdbEntry(
      arpEntry->getIntfID(), arpEntry->getMac());

  auto saiEntry =
      saiManagerTable->neighborManager().saiEntryFromSwEntry(arpEntry);
  auto handle = saiManagerTable->neighborManager().getNeighborHandle(saiEntry);
  // Handle exists (subscription still alive) but neighbor object is gone
  EXPECT_TRUE(handle);
  EXPECT_FALSE(handle->neighbor);
}

TEST_F(ManagedVlanRifNeighborPubSubTest, neighborRecreatedOnFdbReAdd) {
  auto arpEntry = resolveArp(intf0.id, h0);
  EXPECT_TRUE(neighborExists(arpEntry));

  // Remove the FDB entry
  saiManagerTable->fdbManager().removeFdbEntry(
      arpEntry->getIntfID(), arpEntry->getMac());

  auto saiEntry =
      saiManagerTable->neighborManager().saiEntryFromSwEntry(arpEntry);
  auto handle = saiManagerTable->neighborManager().getNeighborHandle(saiEntry);
  EXPECT_TRUE(handle);
  EXPECT_FALSE(handle->neighbor);

  // Re-add the FDB entry. This triggers
  // ManagedVlanRifNeighbor::createObject via pub/sub.
  saiManagerTable->fdbManager().addFdbEntry(
      SaiPortDescriptor(arpEntry->getPort().phyPortID()),
      arpEntry->getIntfID(),
      arpEntry->getMac(),
      SAI_FDB_ENTRY_TYPE_STATIC,
      std::nullopt);

  EXPECT_TRUE(neighborExists(arpEntry));
}

TEST_F(ManagedVlanRifNeighborPubSubTest, linkDownCascadesToNeighbor) {
  auto arpEntry = resolveArp(intf0.id, h0);
  EXPECT_TRUE(neighborExists(arpEntry));

  // Create a next hop that subscribes to the neighbor via pub/sub
  auto swNextHop = makeNextHop(intf0);
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);
  auto* ipNextHop =
      std::get_if<std::shared_ptr<ManagedIpNextHop>>(&managedNextHop);
  ASSERT_NE(ipNextHop, nullptr);
  // Next hop SAI object is alive (neighbor is resolved)
  EXPECT_NE((*ipNextHop)->getSaiObject(), nullptr);

  // handleLinkDown on the FDB manager triggers
  // ManagedFdbEntry::handleLinkDown, which cascades the notification
  // to the FDB publisher, and then to the neighbor subscriber's
  // handleLinkDown.
  saiManagerTable->fdbManager().handleLinkDown(
      SaiPortDescriptor(PortID(h0.port.id)));

  // After linkDown, the neighbor handle still exists
  auto saiEntry =
      saiManagerTable->neighborManager().saiEntryFromSwEntry(arpEntry);
  auto neighborHandle =
      saiManagerTable->neighborManager().getNeighborHandle(saiEntry);
  EXPECT_TRUE(neighborHandle);

  // Next hop SAI object is reset — proves neighbor's handleLinkDown
  // cascaded to its subscriber via SaiNeighborTraits publisher
  EXPECT_EQ((*ipNextHop)->getSaiObject(), nullptr);
}
