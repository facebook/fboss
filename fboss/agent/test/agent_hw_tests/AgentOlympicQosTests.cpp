/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_hw_tests/AgentQosTestBase.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"

namespace facebook::fboss {

class AgentOlympicQosTests : public AgentQosTestBase {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::addOlympicQueueConfig(&cfg, ensemble.getL3Asics());
    utility::addOlympicQosMaps(cfg, ensemble.getL3Asics());
    return cfg;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::L3_QOS,
        production_features::ProductionFeature::OLYMPIC_QOS};
  }
};

// Verify that traffic arriving on a front panel/cpu port is qos mapped to the
// correct queue for each olympic dscp value.
TEST_F(AgentOlympicQosTests, VerifyDscpQueueMapping) {
  verifyDscpQueueMapping(utility::kOlympicQueueToDscp());
}

} // namespace facebook::fboss
