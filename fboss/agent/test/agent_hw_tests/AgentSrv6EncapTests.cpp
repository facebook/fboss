// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/Srv6Tunnel.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class AgentSrv6EncapTest : public AgentHwTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::SRV6_ENCAP};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    addSrv6TunnelConfig(cfg);
    return cfg;
  }

  void setupHelper() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
    resolveNeighborAndProgramRoutes(ecmpHelper, 1);
  }

 private:
  cfg::Srv6Tunnel makeSrv6TunnelConfig(
      const std::string& name,
      InterfaceID interfaceId) const {
    cfg::Srv6Tunnel tunnel;
    tunnel.srv6TunnelId() = name;
    tunnel.underlayIntfID() = interfaceId;
    tunnel.tunnelType() = TunnelType::SRV6_ENCAP;
    tunnel.srcIp() = "2001:db8::1";
    tunnel.ttlMode() = cfg::TunnelMode::PIPE;
    tunnel.dscpMode() = cfg::TunnelMode::UNIFORM;
    tunnel.ecnMode() = cfg::TunnelMode::UNIFORM;
    return tunnel;
  }

  void addSrv6TunnelConfig(cfg::SwitchConfig& cfg) const {
    std::vector<cfg::Srv6Tunnel> tunnelList;
    tunnelList.push_back(makeSrv6TunnelConfig(
        "srv6Tunnel0", InterfaceID(cfg.interfaces()[0].intfID().value())));
    cfg.srv6Tunnels() = tunnelList;
  }
};

TEST_F(AgentSrv6EncapTest, CreateSrv6Tunnel) {
  auto setup = [=, this]() { setupHelper(); };
  auto verify = [=, this]() {
    auto tunnel =
        getProgrammedState()->getSrv6Tunnels()->getNodeIf("srv6Tunnel0");
    ASSERT_NE(tunnel, nullptr);
    EXPECT_EQ(tunnel->getID(), "srv6Tunnel0");
    EXPECT_EQ(tunnel->getType(), TunnelType::SRV6_ENCAP);
    EXPECT_EQ(
        tunnel->getUnderlayIntfId(),
        InterfaceID(initialConfig(*getAgentEnsemble())
                        .interfaces()[0]
                        .intfID()
                        .value()));
    EXPECT_EQ(tunnel->getSrcIP(), folly::IPAddress("2001:db8::1"));
    EXPECT_EQ(tunnel->getDstIP(), std::nullopt);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentSrv6EncapTest, sendPacketToEncapRoute) {
  auto setup = [this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
    resolveNeighbors(ecmpHelper, ecmpHelper.getNextHops().size());

    auto nhop = ecmpHelper.nhop(0);
    std::vector<folly::IPAddressV6> sidList{
        folly::IPAddressV6("3001:db8:1::"),
        folly::IPAddressV6("3001:db8:2::"),
        folly::IPAddressV6("3001:db8:3::")};
    RouteNextHopSet nhops{ResolvedNextHop(
        nhop.ip,
        nhop.intf,
        ECMP_WEIGHT,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        sidList,
        TunnelType::SRV6_ENCAP,
        std::string("srv6Tunnel0"))};
    auto routeUpdater = getSw()->getRouteUpdater();
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2800:2::"),
        64,
        ClientID::BGPD,
        RouteNextHopEntry(nhops, AdminDistance::EBGP));
    routeUpdater.program();
  };

  auto verify = [this]() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
    auto egressPort = ecmpHelper.ecmpPortDescriptorAt(0).phyPortID();

    auto portStatsBefore = getLatestPortStats(egressPort);
    auto bytesBefore = *portStatsBefore.outBytes_();

    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
        getVlanIDForTx(),
        intfMac,
        intfMac,
        folly::IPAddressV6("1::10"),
        folly::IPAddressV6("2800:2::1"),
        8000,
        8001);
    getSw()->sendPacketSwitchedAsync(std::move(txPacket));

    WITH_RETRIES({
      auto portStatsAfter = getLatestPortStats(egressPort);
      auto bytesAfter = *portStatsAfter.outBytes_();
      EXPECT_EVENTUALLY_GT(bytesAfter, bytesBefore);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
