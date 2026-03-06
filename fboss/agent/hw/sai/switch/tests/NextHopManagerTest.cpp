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
#include "fboss/agent/hw/sai/switch/SaiSrv6Manager.h"
#include "fboss/agent/hw/sai/switch/SaiSrv6TunnelManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/Srv6Tunnel.h"

using namespace facebook::fboss;

class NextHopManagerTest : public ManagerTestBase {
 public:
  void checkNextHop(
      NextHopSaiId nextHopId,
      RouterInterfaceSaiId expectedRifId,
      const folly::IPAddress& expectedIp) {
    auto rifIdGot = saiApiTable->nextHopApi().getAttribute(
        nextHopId, SaiIpNextHopTraits::Attributes::RouterInterfaceId());
    EXPECT_EQ(rifIdGot, expectedRifId);
    auto ipGot = saiApiTable->nextHopApi().getAttribute(
        nextHopId, SaiIpNextHopTraits::Attributes::Ip());
    EXPECT_EQ(ipGot, expectedIp);
    auto typeGot = saiApiTable->nextHopApi().getAttribute(
        nextHopId, SaiIpNextHopTraits::Attributes::Type());
    EXPECT_EQ(typeGot, SAI_NEXT_HOP_TYPE_IP);
  }
};

TEST_F(NextHopManagerTest, testAddNextHop) {
  RouterInterfaceSaiId rifId{42};
  folly::IPAddress ip4{"42.42.42.42"};
  std::shared_ptr<SaiIpNextHop> nextHop =
      saiManagerTable->nextHopManager().addNextHop(rifId, ip4);
  checkNextHop(nextHop->adapterKey(), rifId, ip4);
}

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
class Srv6NextHopManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    intf0 = testInterfaces[0];
    resolveArp(intf0.id, intf0.remoteHosts[0]);
  }

  std::shared_ptr<Srv6Tunnel> makeSrv6Tunnel(
      const std::string& tunnelId,
      uint32_t intfID) {
    auto tunnel = std::make_shared<Srv6Tunnel>(tunnelId);
    tunnel->setType(TunnelType::SRV6_ENCAP);
    tunnel->setUnderlayIntfId(InterfaceID(intfID));
    tunnel->setSrcIP(folly::IPAddressV6("2001:db8::1"));
    tunnel->setTTLMode(cfg::TunnelMode::PIPE);
    tunnel->setDscpMode(cfg::TunnelMode::UNIFORM);
    tunnel->setEcnMode(cfg::TunnelMode::UNIFORM);
    return tunnel;
  }

  ResolvedNextHop makeSrv6NextHop(
      const TestInterface& testInterface,
      const std::string& tunnelId) const {
    const auto& remote = testInterface.remoteHosts.at(0);
    std::vector<folly::IPAddressV6> segmentList{
        folly::IPAddressV6("2001:db8::10"), folly::IPAddressV6("2001:db8::20")};
    return ResolvedNextHop{
        remote.ip,
        InterfaceID(testInterface.id),
        ECMP_WEIGHT,
        std::nullopt, // labelForwardingAction
        std::nullopt, // disableTTLDecrement
        std::nullopt, // topologyInfo
        std::nullopt, // adjustedWeight
        segmentList,
        TunnelType::SRV6_ENCAP,
        tunnelId};
  }

  TestInterface intf0;
};

TEST_F(Srv6NextHopManagerTest, getAdapterHostKeySrv6) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
  auto adapterHostKey =
      saiManagerTable->nextHopManager().getAdapterHostKey(swNextHop);

  auto* srv6Key =
      std::get_if<SaiSrv6SidlistNextHopTraits::AdapterHostKey>(&adapterHostKey);
  ASSERT_NE(srv6Key, nullptr);
}

TEST_F(Srv6NextHopManagerTest, getAdapterHostKeyMissingTunnel) {
  auto swNextHop = makeSrv6NextHop(intf0, "nonexistent_tunnel");
  EXPECT_THROW(
      saiManagerTable->nextHopManager().getAdapterHostKey(swNextHop),
      FbossError);
}

TEST_F(Srv6NextHopManagerTest, addManagedSrv6NextHop) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  auto* srv6NextHop =
      std::get_if<std::shared_ptr<ManagedSrv6NextHop>>(&managedNextHop);
  ASSERT_NE(srv6NextHop, nullptr);
  EXPECT_NE(*srv6NextHop, nullptr);
}

TEST_F(Srv6NextHopManagerTest, addManagedSrv6NextHopCreatesSidList) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  auto* srv6NextHop =
      std::get_if<std::shared_ptr<ManagedSrv6NextHop>>(&managedNextHop);
  ASSERT_NE(srv6NextHop, nullptr);
  ASSERT_NE(*srv6NextHop, nullptr);

  // Verify the SID list handle was cached on the managed next hop
  auto& sidListHandle = (*srv6NextHop)->getSrv6SidListHandle();
  ASSERT_NE(sidListHandle, nullptr);
  ASSERT_NE(sidListHandle->sidList, nullptr);

  // Verify SID list attributes
  auto sidListId = sidListHandle->sidList->adapterKey();
  auto gotType = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::Type{});
  EXPECT_EQ(gotType, SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);

  auto gotSegments = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::SegmentList{});
  EXPECT_EQ(gotSegments.size(), 2);
  EXPECT_EQ(gotSegments[0], folly::IPAddressV6("2001:db8::10"));
  EXPECT_EQ(gotSegments[1], folly::IPAddressV6("2001:db8::20"));

  // Verify NextHopId was set on the SID list
  ASSERT_NE((*srv6NextHop)->getSaiObject(), nullptr);
  auto gotNextHopId = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::NextHopId{});
  EXPECT_EQ(gotNextHopId, (*srv6NextHop)->getSaiObject()->adapterKey());
}

TEST_F(Srv6NextHopManagerTest, addManagedSrv6NextHopSidListInSrv6Manager) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  // Get the SID list's AdapterHostKey from the managed next hop
  auto* srv6NextHop =
      std::get_if<std::shared_ptr<ManagedSrv6NextHop>>(&managedNextHop);
  ASSERT_NE(srv6NextHop, nullptr);
  ASSERT_NE(*srv6NextHop, nullptr);
  auto& sidListHandle = (*srv6NextHop)->getSrv6SidListHandle();
  ASSERT_NE(sidListHandle, nullptr);
  ASSERT_NE(sidListHandle->sidList, nullptr);
  auto sidListKey = sidListHandle->sidList->adapterHostKey();

  // Verify the SID list was inserted into SaiSrv6Manager
  auto* handle =
      saiManagerTable->srv6Manager().getSrv6SidListHandle(sidListKey);
  EXPECT_NE(handle, nullptr);
}

TEST_F(Srv6NextHopManagerTest, getManagedSrv6NextHop) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  auto adapterHostKey =
      saiManagerTable->nextHopManager().getAdapterHostKey(swNextHop);
  auto* srv6Key =
      std::get_if<SaiSrv6SidlistNextHopTraits::AdapterHostKey>(&adapterHostKey);
  ASSERT_NE(srv6Key, nullptr);

  auto* got = saiManagerTable->nextHopManager().getManagedNextHop(*srv6Key);
  EXPECT_NE(got, nullptr);
}

TEST_F(Srv6NextHopManagerTest, sidListFreedWhenManagedNextHopDestroyed) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  SaiSrv6SidListTraits::AdapterHostKey sidListKey;

  {
    auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
    auto managedNextHop =
        saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

    // Get the SID list key from the managed next hop
    auto* srv6NextHop =
        std::get_if<std::shared_ptr<ManagedSrv6NextHop>>(&managedNextHop);
    ASSERT_NE(srv6NextHop, nullptr);
    ASSERT_NE(*srv6NextHop, nullptr);
    auto& sidListHandle = (*srv6NextHop)->getSrv6SidListHandle();
    ASSERT_NE(sidListHandle, nullptr);
    sidListKey = sidListHandle->sidList->adapterHostKey();

    // Verify SID list exists in SaiSrv6Manager while managed next hop is alive
    auto* handle =
        saiManagerTable->srv6Manager().getSrv6SidListHandle(sidListKey);
    ASSERT_NE(handle, nullptr);
  }
  // managedNextHop destroyed here — SID list should be freed

  auto* handle =
      saiManagerTable->srv6Manager().getSrv6SidListHandle(sidListKey);
  EXPECT_EQ(handle, nullptr);
}

TEST_F(Srv6NextHopManagerTest, listManagedSrv6NextHops) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  auto output = saiManagerTable->nextHopManager().listManagedObjects();
  EXPECT_FALSE(output.empty());
  EXPECT_NE(output.find("srv6"), std::string::npos);
}
TEST_F(Srv6NextHopManagerTest, linkDownAndReResolveUsesCachedSidList) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
  auto managedNextHop =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  auto* srv6NextHop =
      std::get_if<std::shared_ptr<ManagedSrv6NextHop>>(&managedNextHop);
  ASSERT_NE(srv6NextHop, nullptr);
  ASSERT_NE(*srv6NextHop, nullptr);

  // Record initial SAI object and SID list info
  ASSERT_NE((*srv6NextHop)->getSaiObject(), nullptr);
  auto initialNextHopId = (*srv6NextHop)->getSaiObject()->adapterKey();
  auto& sidListHandle = (*srv6NextHop)->getSrv6SidListHandle();
  ASSERT_NE(sidListHandle, nullptr);
  ASSERT_NE(sidListHandle->sidList, nullptr);
  auto sidListId = sidListHandle->sidList->adapterKey();

  // Verify NextHopId was set on the SID list initially
  auto gotNextHopId = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::NextHopId{});
  EXPECT_EQ(gotNextHopId, initialNextHopId);

  // Simulate link down — cascades through FDB → neighbor → managed next hop
  const auto& remoteHost = intf0.remoteHosts[0];
  saiManagerTable->fdbManager().handleLinkDown(
      SaiPortDescriptor(PortID(remoteHost.port.id)));

  // SAI next hop object should be reset
  EXPECT_EQ((*srv6NextHop)->getSaiObject(), nullptr);

  // NextHopId on the SID list should be cleared to SAI_NULL_OBJECT_ID
  auto clearedNextHopId = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::NextHopId{});
  EXPECT_EQ(clearedNextHopId, SAI_NULL_OBJECT_ID);

  // Remove the neighbor and FDB entry
  auto arpEntry = makeArpEntry(intf0.id, remoteHost);
  saiManagerTable->neighborManager().removeNeighbor(arpEntry);

  // Re-resolve the neighbor — triggers createObject on the managed next hop
  resolveArp(intf0.id, remoteHost);

  // SAI next hop object should be recreated
  ASSERT_NE((*srv6NextHop)->getSaiObject(), nullptr);
  auto newNextHopId = (*srv6NextHop)->getSaiObject()->adapterKey();

  // The cached sidListHandle should still be valid with same sidList object
  auto& cachedHandle = (*srv6NextHop)->getSrv6SidListHandle();
  ASSERT_NE(cachedHandle, nullptr);
  ASSERT_NE(cachedHandle->sidList, nullptr);
  EXPECT_EQ(cachedHandle->sidList->adapterKey(), sidListId);

  // NextHopId on the SID list should be updated to the new next hop
  auto updatedNextHopId = saiApiTable->srv6Api().getAttribute(
      sidListId, SaiSrv6SidListTraits::Attributes::NextHopId{});
  EXPECT_EQ(updatedNextHopId, newNextHopId);
}
#endif
