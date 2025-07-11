// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/agent/test/agent_hw_tests/AgentArsBase.h"

using namespace facebook::fboss::utility;

namespace facebook::fboss {
const int KMaxFlowsetTableSize = 32768;
const int kFlowletTableSize2 = 2048;
folly::CIDRNetwork kAddr2Prefix{folly::IPAddress("2803:6080:d038:3000::"), 64};

static folly::IPAddressV6 kAddr1{"2803:6080:d038:3063::"};
static folly::IPAddressV6 kAddr2{"2803:6080:d038:3000::"};
std::vector<NextHopWeight> swSwitchWeights_ = {
    ECMP_WEIGHT,
    ECMP_WEIGHT,
    ECMP_WEIGHT,
    ECMP_WEIGHT};

using namespace ::testing;
class AgentArsFlowletTest : public AgentArsBase {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::DLB,
    };
  }

 protected:
  void SetUp() override {
    AgentArsBase::SetUp();
  }

  // SAI is multiple of port speed
  int kScalingFactor1() const {
    return getAgentEnsemble()->isSai() ? 10 : 100;
  }

  void addRoute(
      const folly::IPAddressV6 prefix,
      uint8_t mask,
      const std::vector<PortDescriptor>& ports) {
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(
        &wrapper,
        flat_set<PortDescriptor>(
            std::make_move_iterator(ports.begin()),
            std::make_move_iterator(ports.begin() + ports.size())),
        {RoutePrefixV6{prefix, mask}});
  }

  void resolveNextHop(const PortDescriptor& port) {
    applyNewState([this, port](const std::shared_ptr<SwitchState>& state) {
      return helper_->resolveNextHops(state, {port});
    });
  }

  void resolveNextHopsAddRoute(
      const std::vector<PortID>& masterLogicalPortsIds,
      const folly::IPAddressV6 addr) {
    std::vector<PortDescriptor> portDescriptorIds;
    for (auto& portId : masterLogicalPortsIds) {
      this->resolveNextHop(PortDescriptor(portId));
      portDescriptorIds.emplace_back(portId);
    }
    this->addRoute(addr, 64, portDescriptorIds);
  }

  cfg::PortFlowletConfig getPortFlowletConfig(
      int scalingFactor,
      int loadWeight,
      int queueWeight) const {
    cfg::PortFlowletConfig portFlowletConfig;
    portFlowletConfig.scalingFactor() = scalingFactor;
    portFlowletConfig.loadWeight() = loadWeight;
    portFlowletConfig.queueWeight() = queueWeight;
    return portFlowletConfig;
  }

  bool verifyEcmpForFlowletSwitching(
      const folly::CIDRNetwork& ip,
      const cfg::FlowletSwitchingConfig& flowletCfg,
      const bool flowletEnable) {
    AgentEnsemble* ensemble = getAgentEnsemble();
    const auto port = ensemble->masterLogicalPortIds()[0];
    auto switchId = ensemble->scopeResolver().scope(port).switchId();
    auto client = ensemble->getHwAgentTestClient(switchId);
    facebook::fboss::utility::CIDRNetwork cidr;
    cidr.IPAddress() = ip.first.str();
    cidr.mask() = ip.second;
    state::SwitchSettingsFields settings;
    settings.flowletSwitchingConfig() = flowletCfg;
    return client->sync_verifyEcmpForFlowletSwitchingHandler(
        cidr, settings, flowletEnable);
  }
};

TEST_F(AgentArsFlowletTest, ValidateFlowsetExceedForceFix) {
  auto setup = [&]() {
    int numEcmp = int(KMaxFlowsetTableSize / kFlowletTableSize2);
    int totalEcmp = 1;
    for (int i = 1; i < 8 && totalEcmp < numEcmp; i++) {
      for (int j = i + 1; j < 8 && totalEcmp < numEcmp; j++) {
        std::vector<PortID> portIds;
        portIds.push_back(masterLogicalPortIds()[0]);
        portIds.push_back(masterLogicalPortIds()[i]);
        portIds.push_back(masterLogicalPortIds()[j]);
      }
    }
    resolveNextHopsAddRoute(
        {masterLogicalPortIds()[1], masterLogicalPortIds()[2]}, kAddr1);
    resolveNextHopsAddRoute(
        {masterLogicalPortIds()[2], masterLogicalPortIds()[3]}, kAddr2);
  };

  auto verify = [&]() {
    auto cfg = initialConfig(*getAgentEnsemble());
    if (!cfg.flowletSwitchingConfig()) {
      // Skip verification if flowlet switching config is not set
      return;
    }

    // ensure that DLB is not programmed for 17th route as we already have
    // ECMP objects with DLB. Expect flowset size is zero for the 17th
    // object
    if (getAgentEnsemble()->getBootType() != BootType::WARM_BOOT) {
      EXPECT_FALSE(verifyEcmpForFlowletSwitching(
          kAddr2Prefix, // second route
          *cfg.flowletSwitchingConfig(),
          true /* flowletEnable */));
    }
    auto wrapper = getSw()->getRouteUpdater();
    helper_->unprogramRoutes(&wrapper, {RoutePrefixV6{kAddr1, 64}});
    EXPECT_TRUE(verifyEcmpForFlowletSwitching(
        kAddr2Prefix, *cfg.flowletSwitchingConfig(), true /* flowletEnable */));
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
