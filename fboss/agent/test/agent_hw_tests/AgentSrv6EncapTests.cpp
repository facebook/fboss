// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/Srv6Tunnel.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

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

} // namespace facebook::fboss
