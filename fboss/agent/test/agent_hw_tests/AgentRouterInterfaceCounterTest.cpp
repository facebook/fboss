// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"

namespace facebook::fboss {
using facebook::fboss::utility::PfcBufferParams;

class AgentRouterInterfaceCounterTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
    return config;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::ROUTER_INTERFACE_COUNTERS};
  }

 protected:
  void pumpTraffic(bool isV6) {
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcIp = folly::IPAddress(isV6 ? "1001::1" : "10.0.0.1");
    auto dstIp = folly::IPAddress(isV6 ? "100:100:100::1" : "100.100.100.1");
    auto pkt = utility::makeUDPTxPacket(
        getSw(), vlanId, intfMac, intfMac, srcIp, dstIp, 10000, 10001);
    getSw()->sendPacketOutOfPortAsync(
        std::move(pkt), PortID(masterLogicalInterfacePortIds()[0]));
  }
};

TEST_F(AgentRouterInterfaceCounterTest, NoRoute) {
  auto setup = [&]() {
    if (isSupportedOnAllAsics(HwAsic::Feature::PFC)) {
      // Setup buffer configurations only if PFC is supported
      cfg::SwitchConfig cfg = initialConfig(*getAgentEnsemble());
      std::vector<PortID> portIds = {masterLogicalInterfacePortIds()[0]};
      std::vector<int> losslessPgIds = {2};
      std::vector<int> lossyPgIds = {0};
      // Make sure default traffic goes to PG2, which is lossless
      const std::map<int, int> tcToPgOverride{{0, 2}};
      utility::setupPfcBuffers(
          getAgentEnsemble(),
          cfg,
          portIds,
          losslessPgIds,
          lossyPgIds,
          tcToPgOverride);
      applyNewConfig(cfg);
    }
  };
  PortID portId = masterLogicalInterfacePortIds()[0];
  auto verify = [=, this]() {
    auto ensemble = getAgentEnsemble();
    auto state = ensemble->getProgrammedState();
    auto intfID = state->getPorts()->getNode(portId)->getInterfaceID();
    auto statsBefore = ensemble->getLatestInterfaceStats(intfID);
    pumpTraffic(true);
    pumpTraffic(false);
    auto statsAfter = ensemble->getLatestInterfaceStats(intfID);
    EXPECT_GE(*statsAfter.outErrorPkts_(), *statsBefore.outErrorPkts_());
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
