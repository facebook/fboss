/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/EncapIndexAllocator.h"
#include "fboss/agent/hw/sai/switch/SaiLagManager.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;

/*
 * In these tests, we will assume 4 ports with one lane each, with IDs
 */

class NextHopGroupManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE |
        SetupStage::SYSTEM_PORT;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    h0 = intf0.remoteHosts[0];
    intf1 = testInterfaces[1];
    h1 = intf1.remoteHosts[0];
  }

  void checkNextHopGroup(
      NextHopGroupSaiId nextHopGroupId,
      const std::unordered_set<folly::IPAddress>& expectedNextHopIps) {
    auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
    auto& nextHopApi = saiApiTable->nextHopApi();
    SaiNextHopGroupTraits::Attributes::NextHopMemberList memberList{};
    auto members = nextHopGroupApi.getAttribute(nextHopGroupId, memberList);
    EXPECT_EQ(members.size(), expectedNextHopIps.size());

    std::unordered_set<folly::IPAddress> gotNextHopIps;
    for (const auto& member : members) {
      auto nextHopId = nextHopGroupApi.getAttribute(
          NextHopGroupMemberSaiId(member),
          SaiNextHopGroupMemberTraits::Attributes::NextHopId{});
      folly::IPAddress ip = nextHopApi.getAttribute(
          NextHopSaiId(nextHopId), SaiIpNextHopTraits::Attributes::Ip{});
      EXPECT_TRUE(gotNextHopIps.insert(ip).second);
    }
    EXPECT_EQ(gotNextHopIps, expectedNextHopIps);
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
  // Every PRIMARY member of the group must monitor expectedMonitoredObj. Fails
  // if the group has no PRIMARY member at all, so a caller cannot pass
  // vacuously on a group whose members were never created.
  void expectPrimaryMonitoredObject(
      NextHopGroupSaiId nextHopGroupId,
      sai_object_id_t expectedMonitoredObj) {
    auto& nhgApi = saiApiTable->nextHopGroupApi();
    auto members = nhgApi.getAttribute(
        nextHopGroupId, SaiNextHopGroupTraits::Attributes::NextHopMemberList{});
    bool sawPrimary = false;
    for (auto member : members) {
      NextHopGroupMemberSaiId memberId{member};
      if (nhgApi.getAttribute(
              memberId,
              SaiNextHopGroupMemberTraits::Attributes::ConfiguredRole{}) !=
          SAI_NEXT_HOP_GROUP_MEMBER_CONFIGURED_ROLE_PRIMARY) {
        continue;
      }
      sawPrimary = true;
      EXPECT_EQ(
          static_cast<sai_object_id_t>(nhgApi.getAttribute(
              memberId,
              SaiNextHopGroupMemberTraits::Attributes::MonitoredObject{})),
          expectedMonitoredObj);
    }
    EXPECT_TRUE(sawPrimary);
  }
#endif

  TestInterface intf0;
  TestRemoteHost h0;
  TestInterface intf1;
  TestRemoteHost h1;
};

TEST_F(NextHopGroupManagerTest, addNextHopGroup) {
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager()
          .incRefOrAddNextHopGroup(SaiNextHopGroupKey(swNextHops, std::nullopt))
          ->nextHopGroup;
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);
  checkNextHopGroup(saiNextHopGroup->adapterKey(), {});
}

TEST_F(NextHopGroupManagerTest, rejectMultiplePrimariesWithBackup) {
  ResolvedNextHop primaryNh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop primaryNh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  ResolvedNextHop backupNh{
      h0.ip,
      InterfaceID(intf0.id),
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      std::nullopt,
      NextHopRole::BACKUP};
  RouteNextHopEntry::NextHopSet swNextHops{primaryNh1, primaryNh2, backupNh};

  EXPECT_THROW(
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(swNextHops, std::nullopt)),
      FbossError);
}

TEST_F(NextHopGroupManagerTest, verifyNextHopGroupKey) {
  FLAGS_flowletSwitchingEnable = true;
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroupHandle =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(swNextHops, cfg::SwitchingMode::FIXED_ASSIGNMENT));
  auto saiNextHopGroup = saiNextHopGroupHandle->nextHopGroup;
  EXPECT_EQ(
      saiNextHopGroupHandle->desiredEcmpSwitchingMode_,
      cfg::SwitchingMode::FIXED_ASSIGNMENT);
  EXPECT_EQ(saiNextHopGroupHandle.use_count(), 1);
  EXPECT_EQ(saiNextHopGroup.use_count(), 2);

  auto saiNextHopGroupHandle2 =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(swNextHops, cfg::SwitchingMode::FIXED_ASSIGNMENT));
  auto saiNextHopGroup2 = saiNextHopGroupHandle2->nextHopGroup;
  EXPECT_EQ(saiNextHopGroupHandle2.use_count(), 2);
  EXPECT_EQ(saiNextHopGroup2.use_count(), 3);

  EXPECT_EQ(saiNextHopGroup, saiNextHopGroup2);
  EXPECT_EQ(saiNextHopGroupHandle, saiNextHopGroupHandle2);

  auto saiNextHopGroupHandle3 =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(
              swNextHops, cfg::SwitchingMode::PER_PACKET_RANDOM));
  auto saiNextHopGroup3 = saiNextHopGroupHandle3->nextHopGroup;
  EXPECT_EQ(
      saiNextHopGroupHandle3->desiredEcmpSwitchingMode_,
      cfg::SwitchingMode::PER_PACKET_RANDOM);
  EXPECT_EQ(saiNextHopGroupHandle3.use_count(), 1);
  EXPECT_EQ(saiNextHopGroup3.use_count(), 2);

  EXPECT_NE(saiNextHopGroupHandle3, saiNextHopGroupHandle);
  EXPECT_NE(saiNextHopGroup3, saiNextHopGroup);

  EXPECT_EQ(saiNextHopGroupHandle2.use_count(), 2);
  EXPECT_EQ(saiNextHopGroup2.use_count(), 3);
}

TEST_F(NextHopGroupManagerTest, refNextHopGroup) {
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager()
          .incRefOrAddNextHopGroup(SaiNextHopGroupKey(swNextHops, std::nullopt))
          ->nextHopGroup;
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);

  RouteNextHopEntry::NextHopSet swNextHops2{nh1, nh2};
  auto saiNextHopGroup2 = saiManagerTable->nextHopGroupManager()
                              .incRefOrAddNextHopGroup(
                                  SaiNextHopGroupKey(swNextHops2, std::nullopt))
                              ->nextHopGroup;
  EXPECT_EQ(saiNextHopGroup.use_count(), 2);

  EXPECT_EQ(saiNextHopGroup, saiNextHopGroup2);
  checkNextHopGroup(saiNextHopGroup->adapterKey(), {});
}

TEST_F(NextHopGroupManagerTest, derefNextHopGroup) {
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroup =
      saiManagerTable->nextHopGroupManager()
          .incRefOrAddNextHopGroup(SaiNextHopGroupKey(swNextHops, std::nullopt))
          ->nextHopGroup;
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);
  {
    RouteNextHopEntry::NextHopSet swNextHops2{nh1, nh2};
    auto saiNextHopGroup2 = saiManagerTable->nextHopGroupManager()
                                .incRefOrAddNextHopGroup(SaiNextHopGroupKey(
                                    swNextHops2, std::nullopt))
                                ->nextHopGroup;
    EXPECT_EQ(saiNextHopGroup.use_count(), 2);
  }
  EXPECT_EQ(saiNextHopGroup.use_count(), 1);
  checkNextHopGroup(saiNextHopGroup->adapterKey(), {});
}

TEST_F(NextHopGroupManagerTest, deleteNextHopGroup) {
  std::weak_ptr<SaiNextHopGroup> counter;
  {
    ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
    ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
    RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
    auto saiNextHopGroup = saiManagerTable->nextHopGroupManager()
                               .incRefOrAddNextHopGroup(
                                   SaiNextHopGroupKey(swNextHops, std::nullopt))
                               ->nextHopGroup;
    counter = saiNextHopGroup;
    EXPECT_EQ(counter.use_count(), 1);
  }
  EXPECT_EQ(counter.use_count(), 0);
  EXPECT_TRUE(counter.expired());
}

TEST_F(NextHopGroupManagerTest, resolveNeighborBefore) {
  auto arpEntry0 = resolveArp(intf0.id, h0);
  auto arpEntry1 = resolveArp(intf1.id, h1);
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroupHandle =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(swNextHops, std::nullopt));
  auto saiNextHopGroup = saiNextHopGroupHandle->nextHopGroup;
  checkNextHopGroup(saiNextHopGroup->adapterKey(), {h0.ip, h1.ip});
}

TEST_F(NextHopGroupManagerTest, resolveNeighborAfter) {
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroupHandle =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(swNextHops, std::nullopt));
  auto saiNextHopGroup = saiNextHopGroupHandle->nextHopGroup;
  checkNextHopGroup(saiNextHopGroup->adapterKey(), {});
  auto arpEntry0 = resolveArp(intf0.id, h0);
  auto arpEntry1 = resolveArp(intf1.id, h1);
  checkNextHopGroup(saiNextHopGroup->adapterKey(), {h0.ip, h1.ip});
}

TEST_F(NextHopGroupManagerTest, unresolveNeighbor) {
  auto arpEntry0 = resolveArp(intf0.id, h0);
  auto arpEntry1 = resolveArp(intf1.id, h1);
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroupHandle =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(swNextHops, std::nullopt));
  auto saiNextHopGroup = saiNextHopGroupHandle->nextHopGroup;
  checkNextHopGroup(saiNextHopGroup->adapterKey(), {h0.ip, h1.ip});
  saiManagerTable->neighborManager().removeNeighbor(arpEntry1);
  checkNextHopGroup(saiNextHopGroup->adapterKey(), {h0.ip});
  saiManagerTable->neighborManager().removeNeighbor(arpEntry0);
  checkNextHopGroup(saiNextHopGroup->adapterKey(), {});
}

TEST_F(NextHopGroupManagerTest, derefThenResolve) {
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  {
    auto saiNextHopGroupHandle =
        saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
            SaiNextHopGroupKey(swNextHops, std::nullopt));
    auto saiNextHopGroup = saiNextHopGroupHandle->nextHopGroup;
    checkNextHopGroup(saiNextHopGroup->adapterKey(), {});
  }
  auto arpEntry0 = makeArpEntry(intf0.id, h0);
  saiManagerTable->neighborManager().addNeighbor(arpEntry0);
  auto arpEntry1 = makeArpEntry(intf1.id, h1);
  saiManagerTable->neighborManager().addNeighbor(arpEntry1);
  // Assertion is that nothing crashes because we _would_ have processed
  // these resolutions but the object has been deleted
}

TEST_F(NextHopGroupManagerTest, testNextHopGroupMemberWeights) {
  resolveArp(intf0.id, h0);
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), 42};
  RouteNextHopEntry::NextHopSet swNextHops{nh1};
  auto saiNextHopGroupHandle =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(swNextHops, std::nullopt));
  auto saiNextHopGroup = saiNextHopGroupHandle->nextHopGroup;

  auto nextHopGroupId = saiNextHopGroup->adapterKey();
  auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
  SaiNextHopGroupTraits::Attributes::NextHopMemberList memberList{};
  auto members = nextHopGroupApi.getAttribute(nextHopGroupId, memberList);
  EXPECT_EQ(members.size(), 1);
  auto weight = nextHopGroupApi.getAttribute(
      NextHopGroupMemberSaiId(members[0]),
      SaiNextHopGroupMemberTraits::Attributes::Weight{});
  EXPECT_EQ(weight, 42);
}

TEST_F(NextHopGroupManagerTest, testFixedWidthNextHopGroupMemberWeights) {
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  FLAGS_ecmp_width = 512;
  resolveArp(intf0.id, h0);
  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), 64};
  resolveArp(intf1.id, h1);
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), 65};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};
  auto saiNextHopGroupHandle =
      saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
          SaiNextHopGroupKey(swNextHops, std::nullopt));
  auto saiNextHopGroup = saiNextHopGroupHandle->nextHopGroup;

  auto nextHopGroupId = saiNextHopGroup->adapterKey();
  auto& nextHopGroupApi = saiApiTable->nextHopGroupApi();
  SaiNextHopGroupTraits::Attributes::NextHopMemberList memberList{};
  auto members = nextHopGroupApi.getAttribute(nextHopGroupId, memberList);
  EXPECT_EQ(members.size(), 2);
  auto newWeight = nextHopGroupApi.getAttribute(
                       NextHopGroupMemberSaiId(members[0]),
                       SaiNextHopGroupMemberTraits::Attributes::Weight{}) +
      nextHopGroupApi.getAttribute(
          NextHopGroupMemberSaiId(members[1]),
          SaiNextHopGroupMemberTraits::Attributes::Weight{});
  EXPECT_EQ(newWeight, 512);
#endif
}

#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
namespace {
ResolvedNextHop makeBackupNextHop(
    const folly::IPAddress& ip,
    InterfaceID intf) {
  return ResolvedNextHop{
      ip,
      intf,
      ECMP_WEIGHT,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      std::nullopt,
      NextHopRole::BACKUP};
}
} // namespace

// A PROTECTION group's PRIMARY member monitors its own egress port/LAG, so
// that port going down drives the ASIC's autonomous switchover to the standby
// group. The object is derived from the member's next hop rather than passed
// in, and is only programmed on ASICs that cannot infer it themselves.
TEST_F(NextHopGroupManagerTest, protectionGroupPrimaryMonitorsItsEgressPort) {
  resolveArp(intf0.id, h0);
  resolveArp(intf1.id, h1);

  ResolvedNextHop primaryNh{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{
      primaryNh, makeBackupNextHop(h1.ip, InterfaceID(intf1.id))};

  auto handle = saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
      SaiNextHopGroupKey(
          swNextHops, std::nullopt, SAI_NEXT_HOP_GROUP_TYPE_PROTECTION));
  ASSERT_NE(handle, nullptr);
  ASSERT_NE(handle->nextHopGroup, nullptr);

  // The primary's neighbor egresses this port, so this is what it must monitor.
  auto* portHandle =
      saiManagerTable->portManager().getPortHandle(PortID(h0.port.id));
  ASSERT_NE(portHandle, nullptr);
  auto expectedMonitoredObj =
      static_cast<sai_object_id_t>(portHandle->port->adapterKey());

  auto& nhgApi = saiApiTable->nextHopGroupApi();
  EXPECT_EQ(
      nhgApi.getAttribute(
          handle->nextHopGroup->adapterKey(),
          SaiNextHopGroupTraits::Attributes::Type{}),
      SAI_NEXT_HOP_GROUP_TYPE_PROTECTION);

  expectPrimaryMonitoredObject(
      handle->nextHopGroup->adapterKey(), expectedMonitoredObj);
}

// Same as above, but the neighbor resolves AFTER the group is created -- the
// ordinary steady-state ordering, where a route/MySid is programmed before ARP
// completes. The PRIMARY member is created during neighbor resolution and must
// still come up monitoring the right port.
TEST_F(
    NextHopGroupManagerTest,
    protectionGroupPrimaryMonitorsPortResolvedLater) {
  ResolvedNextHop primaryNh{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{
      primaryNh, makeBackupNextHop(h1.ip, InterfaceID(intf1.id))};

  auto handle = saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
      SaiNextHopGroupKey(
          swNextHops, std::nullopt, SAI_NEXT_HOP_GROUP_TYPE_PROTECTION));
  ASSERT_NE(handle, nullptr);
  ASSERT_NE(handle->nextHopGroup, nullptr);

  // Now resolve, which is what actually creates the members.
  resolveArp(intf0.id, h0);
  resolveArp(intf1.id, h1);

  auto* portHandle =
      saiManagerTable->portManager().getPortHandle(PortID(h0.port.id));
  ASSERT_NE(portHandle, nullptr);
  auto expectedMonitoredObj =
      static_cast<sai_object_id_t>(portHandle->port->adapterKey());

  expectPrimaryMonitoredObject(
      handle->nextHopGroup->adapterKey(), expectedMonitoredObj);
}

// The derivation is scoped to protection groups: an ordinary ECMP group's
// members are neither tagged PRIMARY nor given a monitored object.
TEST_F(NextHopGroupManagerTest, ecmpGroupMembersHaveNoMonitoredObject) {
  resolveArp(intf0.id, h0);
  resolveArp(intf1.id, h1);

  ResolvedNextHop nh1{h0.ip, InterfaceID(intf0.id), ECMP_WEIGHT};
  ResolvedNextHop nh2{h1.ip, InterfaceID(intf1.id), ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{nh1, nh2};

  auto handle = saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
      SaiNextHopGroupKey(swNextHops, std::nullopt));
  ASSERT_NE(handle, nullptr);

  auto& nhgApi = saiApiTable->nextHopGroupApi();
  auto members = nhgApi.getAttribute(
      handle->nextHopGroup->adapterKey(),
      SaiNextHopGroupTraits::Attributes::NextHopMemberList{});
  ASSERT_FALSE(members.empty());
  for (auto member : members) {
    NextHopGroupMemberSaiId memberId{member};
    EXPECT_EQ(
        static_cast<sai_object_id_t>(nhgApi.getAttribute(
            memberId,
            SaiNextHopGroupMemberTraits::Attributes::MonitoredObject{})),
        SAI_NULL_OBJECT_ID);
  }
}

// On an ASIC that cannot infer the monitored object, failing to derive it must
// fail the member create: a protection group that looks programmed but can
// never switch over is worse than a rejected update.
TEST_F(NextHopGroupManagerTest, monitoredObjectThrowsWhenEgressUnknown) {
  auto& neighborManager = saiManagerTable->neighborManager();
  auto* rifHandle =
      saiManagerTable->routerInterfaceManager().getRouterInterfaceHandle(
          InterfaceID(intf0.id));
  ASSERT_NE(rifHandle, nullptr);
  // A neighbor that was never resolved, so it has no egress port.
  SaiNeighborTraits::NeighborEntry unresolved(
      neighborManager.getSwitchSaiId(), rifHandle->adapterKey(), h0.ip);

  EXPECT_THROW(
      saiManagerTable->nextHopGroupManager().getMonitoredObjectIf(unresolved),
      FbossError);
}
#endif

#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
// PortRif-path neighbor (SYSTEM_PORT rif) resolving AFTER the protection group
// exists. PortRifNeighbor publishes its SAI object from its constructor, so
// member creation cascades before SaiNeighborManager records the neighbor.
TEST_F(NextHopGroupManagerTest, portRifPrimaryMonitorsPortResolvedLater) {
  const auto sysPortIntfId =
      getIntfID(intf0.id, cfg::InterfaceType::SYSTEM_PORT);
  ResolvedNextHop primaryNh{h0.ip, sysPortIntfId, ECMP_WEIGHT};
  RouteNextHopEntry::NextHopSet swNextHops{
      primaryNh, makeBackupNextHop(h1.ip, InterfaceID(intf1.id))};

  auto handle = saiManagerTable->nextHopGroupManager().incRefOrAddNextHopGroup(
      SaiNextHopGroupKey(
          swNextHops, std::nullopt, SAI_NEXT_HOP_GROUP_TYPE_PROTECTION));
  ASSERT_NE(handle, nullptr);
  ASSERT_NE(handle->nextHopGroup, nullptr);

  auto encapIndex = EncapIndexAllocator::getNextAvailableEncapIdx(
      programmedState, *saiPlatform->getAsic());
  resolveArp(
      intf0.id, h0, cfg::InterfaceType::SYSTEM_PORT, std::nullopt, encapIndex);

  // The neighbor's port descriptor is the physical egress port even though its
  // rif is a SYSTEM_PORT rif, so the monitored object is that port, not the
  // system port object.
  auto* portHandle =
      saiManagerTable->portManager().getPortHandle(PortID(h0.port.id));
  ASSERT_NE(portHandle, nullptr);
  auto expectedMonitoredObj =
      static_cast<sai_object_id_t>(portHandle->port->adapterKey());

  expectPrimaryMonitoredObject(
      handle->nextHopGroup->adapterKey(), expectedMonitoredObj);
}
#endif
