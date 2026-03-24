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
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

/*
 * Tests for ManagedNextHop pub/sub behavior.
 *
 * ManagedNextHop<SaiIpNextHopTraits> subscribes to SaiNeighborTraits publisher.
 * When the neighbor appears, ManagedNextHop::createObject creates
 * the SAI next hop. When the neighbor disappears,
 * ManagedNextHop::removeObject cleans up the next hop.
 * On handleLinkDown, the next hop is reset.
 */
class ManagedNextHopPubSubTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    h0 = intf0.remoteHosts[0];
  }

  TestInterface intf0;
  TestRemoteHost h0;
};

TEST_F(ManagedNextHopPubSubTest, nextHopCreatedViaNeighborPubSub) {
  // Resolve neighbor first (creates neighbor publisher)
  auto arpEntry = resolveArp(intf0.id, h0);

  // Create a managed next hop (subscribes to neighbor publisher)
  ResolvedNextHop swNextHop{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  // The neighbor already exists, so createObject should have been called
  // immediately via pub/sub, creating the SAI next hop object.
  auto* ipNextHop =
      std::get_if<std::shared_ptr<ManagedIpNextHop>>(&managedNextHop);
  ASSERT_NE(ipNextHop, nullptr);
  EXPECT_NE((*ipNextHop)->getSaiObject(), nullptr);
}

TEST_F(ManagedNextHopPubSubTest, nextHopNotCreatedWithoutNeighbor) {
  // Create managed next hop without resolving neighbor first
  ResolvedNextHop swNextHop{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  // No neighbor exists yet, so SAI next hop should not be created
  auto* ipNextHop =
      std::get_if<std::shared_ptr<ManagedIpNextHop>>(&managedNextHop);
  ASSERT_NE(ipNextHop, nullptr);
  EXPECT_EQ((*ipNextHop)->getSaiObject(), nullptr);
}

TEST_F(ManagedNextHopPubSubTest, nextHopCreatedWhenNeighborResolvesLater) {
  // Create managed next hop first (subscriber)
  ResolvedNextHop swNextHop{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  auto* ipNextHop =
      std::get_if<std::shared_ptr<ManagedIpNextHop>>(&managedNextHop);
  ASSERT_NE(ipNextHop, nullptr);
  EXPECT_EQ((*ipNextHop)->getSaiObject(), nullptr);

  // Now resolve the neighbor (creates publisher)
  resolveArp(intf0.id, h0);

  // Pub/sub should have triggered createObject
  EXPECT_NE((*ipNextHop)->getSaiObject(), nullptr);
}

TEST_F(ManagedNextHopPubSubTest, nextHopRemovedOnNeighborRemoval) {
  auto arpEntry = resolveArp(intf0.id, h0);

  ResolvedNextHop swNextHop{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  auto* ipNextHop =
      std::get_if<std::shared_ptr<ManagedIpNextHop>>(&managedNextHop);
  ASSERT_NE(ipNextHop, nullptr);
  EXPECT_NE((*ipNextHop)->getSaiObject(), nullptr);

  // Remove neighbor. This triggers ManagedNextHop::removeObject via pub/sub.
  saiManagerTable->neighborManager().removeNeighbor(arpEntry);

  EXPECT_EQ((*ipNextHop)->getSaiObject(), nullptr);
}

TEST_F(ManagedNextHopPubSubTest, nextHopResetOnLinkDown) {
  auto arpEntry = resolveArp(intf0.id, h0);

  ResolvedNextHop swNextHop{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  auto* ipNextHop =
      std::get_if<std::shared_ptr<ManagedIpNextHop>>(&managedNextHop);
  ASSERT_NE(ipNextHop, nullptr);
  EXPECT_NE((*ipNextHop)->getSaiObject(), nullptr);

  // handleLinkDown cascades from FDB → neighbor → next hop
  saiManagerTable->fdbManager().handleLinkDown(
      SaiPortDescriptor(PortID(h0.port.id)));

  // handleLinkDown resets the SAI next hop object
  EXPECT_EQ((*ipNextHop)->getSaiObject(), nullptr);
}
