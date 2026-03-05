/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
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

TEST_F(Srv6NextHopManagerTest, addManagedSrv6NextHopRefCount) {
  auto swTunnel = makeSrv6Tunnel("srv6tunnel0", intf0.id);
  saiManagerTable->srv6TunnelManager().addSrv6Tunnel(swTunnel);

  auto swNextHop = makeSrv6NextHop(intf0, "srv6tunnel0");
  auto managedNextHop1 =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);
  auto managedNextHop2 =
      saiManagerTable->nextHopManager().addManagedSaiNextHop(swNextHop);

  auto* srv6NextHop1 =
      std::get_if<std::shared_ptr<ManagedSrv6NextHop>>(&managedNextHop1);
  auto* srv6NextHop2 =
      std::get_if<std::shared_ptr<ManagedSrv6NextHop>>(&managedNextHop2);
  ASSERT_NE(srv6NextHop1, nullptr);
  ASSERT_NE(srv6NextHop2, nullptr);
  EXPECT_EQ(*srv6NextHop1, *srv6NextHop2);
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
#endif
