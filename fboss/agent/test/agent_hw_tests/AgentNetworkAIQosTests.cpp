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
#include "fboss/agent/test/agent_hw_tests/AgentQosTestBase.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/NetworkAITestUtils.h"

namespace facebook::fboss {

class AgentNetworkAIQosTests : public AgentQosTestBase {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    auto hwAsic = checkSameAndGetAsic(ensemble.getL3Asics());
    auto streamType =
        *hwAsic->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT).begin();
    utility::addNetworkAIQueueConfig(
        &cfg, streamType, cfg::QueueScheduling::STRICT_PRIORITY, hwAsic);
    utility::addNetworkAIQosMaps(cfg, ensemble.getL3Asics());
    if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      utility::setDefaultCpuTrafficPolicyConfig(
          cfg, ensemble.getL3Asics(), ensemble.isSai());
      utility::addCpuQueueConfig(cfg, ensemble.getL3Asics(), ensemble.isSai());
      utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), cfg);
    }
    return cfg;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_QOS, ProductionFeature::NETWORKAI_QOS};
  }
};

// Verify that traffic arriving on a front panel/cpu port is qos mapped to the
// correct queue for each olympic dscp value.
TEST_F(AgentNetworkAIQosTests, VerifyDscpQueueMapping) {
  verifyDscpQueueMapping(utility::kNetworkAIQueueToDscp());
}

} // namespace facebook::fboss
