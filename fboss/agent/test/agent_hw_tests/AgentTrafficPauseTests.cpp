// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"

namespace facebook::fboss {

class AgentTrafficPauseTest : public AgentHwTest {
 public:
  const folly::IPAddressV6 kDestIp{
      folly::IPAddressV6("2620:0:1cfe:face:b00c::4")};

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
    return config;
  }

  void setupEcmpTraffic(const PortID& portId) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper{
        getProgrammedState(),
        utility::getFirstInterfaceMac(getProgrammedState())};

    const PortDescriptor port(portId);
    RoutePrefixV6 route{kDestIp, 128};

    applyNewState([&](const std::shared_ptr<SwitchState>& state) {
      return ecmpHelper.resolveNextHops(state, {port});
    });

    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&routeUpdater, {port}, {route});

    utility::ttlDecrementHandlingForLoopbackTraffic(
        getAgentEnsemble(), ecmpHelper.getRouterId(), ecmpHelper.nhop(port));
  }

  void sendPauseFrames(const PortID& portId, const int count) {
    // PAUSE frame to have the highest quanta of 0xffff
    std::vector<uint8_t> payload{0x00, 0x01, 0xff, 0xff};
    std::vector<uint8_t> padding(42, 0);
    payload.insert(payload.end(), padding.begin(), padding.end());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    for (int idx = 0; idx < count; idx++) {
      auto pkt = utility::makeEthTxPacket(
          getSw(),
          utility::firstVlanID(getProgrammedState()),
          utility::MacAddressGenerator().get(intfMac.u64NBO() + 1),
          folly::MacAddress("01:80:C2:00:00:01"),
          ETHERTYPE::ETHERTYPE_EPON,
          payload);
      // Inject pause frames on highest priority queue to avoid drops
      getSw()->sendPacketOutOfPortAsync(
          std::move(pkt),
          portId,
          utility::getOlympicQueueId(utility::OlympicQueueType::NC));
    }
  }

  void pumpTraffic(const PortID& portId) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto dscp = utility::kOlympicQueueToDscp().at(0).front();
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    for (int i = 0; i < getAgentEnsemble()->getMinPktsForLineRate(portId);
         i++) {
      auto txPacket = utility::makeUDPTxPacket(
          getSw(),
          vlanId,
          srcMac,
          intfMac,
          folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
          kDestIp,
          8000,
          8001,
          dscp << 2,
          255,
          std::vector<uint8_t>(2000, 0xff));
      getAgentEnsemble()->sendPacketAsync(
          std::move(txPacket), PortDescriptor(portId), std::nullopt);
    }
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::PAUSE};
  }
};

} // namespace facebook::fboss
