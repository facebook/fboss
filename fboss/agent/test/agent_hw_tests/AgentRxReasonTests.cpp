/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"

namespace facebook::fboss {

class AgentRxReasonTests : public AgentHwTest {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);

    utility::setDefaultCpuTrafficPolicyConfig(
        config, ensemble.getL3Asics(), ensemble.isSai());
    // Remove DHCP from rxReason list
    auto rxReasonListWithoutDHCP = utility::getCoppRxReasonToQueues(
        ensemble.getL3Asics(), ensemble.isSai());
    auto dhcpRxReason = ControlPlane::makeRxReasonToQueueEntry(
        cfg::PacketRxReason::DHCP,
        utility::getCoppMidPriQueueId(ensemble.getL3Asics()));
    auto dhcpRxReasonIter = std::find(
        rxReasonListWithoutDHCP.begin(),
        rxReasonListWithoutDHCP.end(),
        dhcpRxReason);
    if (dhcpRxReasonIter != rxReasonListWithoutDHCP.end()) {
      rxReasonListWithoutDHCP.erase(dhcpRxReasonIter);
    }
    config.cpuTrafficPolicy()->rxReasonToQueueOrderedList() =
        rxReasonListWithoutDHCP;
    return config;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::COPP};
  }

  // Because number of RX reasons is modified across WB, repopulating priority
  // is expected.
  bool failHwCallsOnWarmboot() const override {
    return false;
  }
};

TEST_F(AgentRxReasonTests, InsertAndRemoveRxReason) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    applyNewConfig(config);
    config.cpuTrafficPolicy()->rxReasonToQueueOrderedList() =
        utility::getCoppRxReasonToQueues(
            getAgentEnsemble()->getL3Asics(), getAgentEnsemble()->isSai());
    applyNewConfig(config);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
