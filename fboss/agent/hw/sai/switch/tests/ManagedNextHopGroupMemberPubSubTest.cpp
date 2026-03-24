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
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

/*
 * Tests for ManagedSaiNextHopGroupMember pub/sub behavior.
 *
 * ManagedSaiNextHopGroupMember subscribes to NextHopTraits publisher
 * (SaiIpNextHopTraits). When the next hop SAI object appears,
 * createObject creates the group member. When it disappears,
 * removeObject cleans up the group member.
 *
 * The full pub/sub cascade is:
 * BridgePort → FdbEntry → Neighbor → NextHop → NextHopGroupMember
 */
class ManagedNextHopGroupMemberPubSubTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    h0 = intf0.remoteHosts[0];
    intf1 = testInterfaces[1];
    h1 = intf1.remoteHosts[0];
  }

  size_t getNextHopGroupMemberCount(NextHopGroupSaiId groupId) const {
    auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
    auto members = nextHopGroupApi.getAttribute(
        groupId, SaiNextHopGroupTraits::Attributes::NextHopMemberList{});
    return members.size();
  }

  TestInterface intf0;
  TestRemoteHost h0;
  TestInterface intf1;
  TestRemoteHost h1;
};

TEST_F(
    ManagedNextHopGroupMemberPubSubTest,
    groupMemberCreatedWhenNeighborResolved) {
  // Create next hop group first (no neighbors resolved yet)
  ResolvedNextHop nh0{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh1{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh0, nh1};
  auto handle = saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
      SaiNextHopGroupKey(swNextHops, std::nullopt));
  auto groupId = handle->nextHopGroup->adapterKey();

  // No neighbors resolved — group should have 0 members
  EXPECT_EQ(getNextHopGroupMemberCount(groupId), 0);

  // Resolve one neighbor — triggers pub/sub cascade:
  // FDB → neighbor → next hop → group member
  resolveArp(intf0.id, h0);
  EXPECT_EQ(getNextHopGroupMemberCount(groupId), 1);

  // Resolve second neighbor
  resolveArp(intf1.id, h1);
  EXPECT_EQ(getNextHopGroupMemberCount(groupId), 2);
}

TEST_F(
    ManagedNextHopGroupMemberPubSubTest,
    groupMemberRemovedWhenNeighborUnresolved) {
  // Resolve both neighbors first
  auto arpEntry0 = resolveArp(intf0.id, h0);
  auto arpEntry1 = resolveArp(intf1.id, h1);

  // Create next hop group
  ResolvedNextHop nh0{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh1{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh0, nh1};
  auto handle = saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
      SaiNextHopGroupKey(swNextHops, std::nullopt));
  auto groupId = handle->nextHopGroup->adapterKey();
  EXPECT_EQ(getNextHopGroupMemberCount(groupId), 2);

  // Remove one neighbor — triggers pub/sub cascade removing
  // the group member
  saiManagerTable->neighborManager().removeNeighbor(arpEntry0);
  EXPECT_EQ(getNextHopGroupMemberCount(groupId), 1);

  // Remove second neighbor
  saiManagerTable->neighborManager().removeNeighbor(arpEntry1);
  EXPECT_EQ(getNextHopGroupMemberCount(groupId), 0);
}

TEST_F(ManagedNextHopGroupMemberPubSubTest, groupMemberRemovedOnLinkDown) {
  auto arpEntry = resolveArp(intf0.id, h0);

  ResolvedNextHop nh0{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh0};
  auto handle = saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
      SaiNextHopGroupKey(swNextHops, std::nullopt));
  auto groupId = handle->nextHopGroup->adapterKey();
  EXPECT_EQ(getNextHopGroupMemberCount(groupId), 1);

  // LinkDown cascades: FDB → neighbor → next hop → group member
  saiManagerTable->fdbManager().handleLinkDown(
      SaiPortDescriptor(PortID(h0.port.id)));
  EXPECT_EQ(getNextHopGroupMemberCount(groupId), 0);
}
