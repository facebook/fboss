/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"
#include "fboss/agent/test/utils/NetworkAITestUtils.h"
#include "fboss/agent/test/utils/SystemScaleTestUtils.h"

namespace facebook::fboss {

class AgentNSFScaleTest : public AgentHwTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::L3_FORWARDING,
        ProductionFeature::DLB,
        ProductionFeature::ARS_SPRAY,
        ProductionFeature::ERSPANv6_SAMPLING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);

    utility::addFlowletConfigs(
        cfg,
        ensemble.masterLogicalPortIds(),
        ensemble.isSai(),
        cfg::SwitchingMode::PER_PACKET_QUALITY,
        cfg::SwitchingMode::PER_PACKET_RANDOM);
    auto hwAsic = checkSameAndGetAsic(ensemble.getL3Asics());
    auto streamType =
        *hwAsic->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT).begin();
    utility::addNetworkAIQueueConfig(
        &cfg, streamType, cfg::QueueScheduling::STRICT_PRIORITY, hwAsic);
    utility::addNetworkAIQosMaps(cfg, ensemble.getL3Asics());
    utility::setDefaultCpuTrafficPolicyConfig(
        cfg, ensemble.getL3Asics(), ensemble.isSai());
    utility::addCpuQueueConfig(cfg, ensemble.getL3Asics(), ensemble.isSai());
    utility::addMirrorConfig<folly::IPAddressV6>(
        &cfg, ensemble, utility::kIngressErspan, true /* truncate */);
    return cfg;
  }

  void setupSystemScale() {
    auto generator = utility::NSFRouteScaleGenerator(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    utility::initSystemScaleTest(getAgentEnsemble(), generator);
  }

 protected:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_route_resource_protection = false;
    FLAGS_enable_ecmp_resource_manager = true;
    FLAGS_flowletSwitchingEnable = true;
  }
};

TEST_F(AgentNSFScaleTest, Test) {
  verifyAcrossWarmBoots([=, this]() { setupSystemScale(); }, []() {});
}
} // namespace facebook::fboss
