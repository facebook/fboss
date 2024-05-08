/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/test/agent_hw_tests/AgentConfigSetupTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

class AgentConfigVerifyQosTest : public AgentConfigSetupTest {
 private:
  cfg::SwitchConfig getFallbackConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(), ensemble.masterLogicalPortIds());
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::L3_QOS};
  }
};

TEST_F(AgentConfigVerifyQosTest, verifyDscpQueueMapping) {
  auto verifyPostWb = [&]() {
    // TODO: verify that DSCP Queue mapping should honor either 2Q config or
    // Olympic config.
  };

  verifyAcrossWarmBoots(
      testSetup(), testVerify(), testSetupPostWb(), verifyPostWb);
}

} // namespace facebook::fboss
