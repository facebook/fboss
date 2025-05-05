/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional
 *  grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {
class AgentHwParityErrorTest : public AgentHwTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::HW_SWITCH};
  }
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // enable running on fab switches as well
    FLAGS_hide_fabric_ports = false;
  }
};

TEST_F(AgentHwParityErrorTest, verifyParityError) {
  auto setup = [=, this]() {
    applyNewConfig(initialConfig(*getAgentEnsemble()));
  };
  auto verify = [=, this]() {
    const auto kPort = masterLogicalPortIds()[0];
    auto switchId = scopeResolver().scope(kPort).switchId();
    auto switchIndex =
        getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
    auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);
    WITH_RETRIES({
      auto switchStats = getHwSwitchStats(switchIndex);
      auto asicErrors = switchStats.hwAsicErrors().value();
      EXPECT_EVENTUALLY_EQ(*asicErrors.correctedParityErrors(), 0);
    });
    client->sync_triggerParityError();
    WITH_RETRIES({
      auto switchStats = getHwSwitchStats(switchIndex);
      auto asicErrors = switchStats.hwAsicErrors().value();
      EXPECT_EVENTUALLY_GT(*asicErrors.correctedParityErrors(), 0);
    });
  };
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
