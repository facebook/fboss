/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>

#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace {
constexpr int kLoopCount = 10;
} // namespace

namespace facebook::fboss {

class AgentHwPtpTcTest : public AgentHwTest {
 protected:
  void SetUp() override {
    AgentHwTest::SetUp();
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::PTP_TC};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
  }

  void setPtpTc(const bool val) {
    const AgentEnsemble* ensemble = getAgentEnsemble();
    auto cfg{initialConfig(*ensemble)};
    cfg.switchSettings()->ptpTcEnable() = val;
    applyNewConfig(cfg);
  }
};

TEST_F(AgentHwPtpTcTest, VerifyPtpTcEnable) {
  AgentEnsemble* ensemble = getAgentEnsemble();
  auto l3Asics = ensemble->getL3Asics();
  auto switchId = getSw()
                      ->getScopeResolver()
                      ->scope(ensemble->masterLogicalPortIds())
                      .switchId();
  auto client = ensemble->getHwAgentTestClient(switchId);
  XLOG(DBG2) << "starting VerifyPtpTcEnable test on switchId=" << switchId;

  auto setup = [=, this]() {
    auto cfg = initialConfig(*ensemble);
    applyNewConfig(cfg);
    setPtpTc(true);
  };
  auto verify = [&]() { EXPECT_TRUE(client->sync_getPtpTcEnabled()); };
  auto verifyPostWarmboot = [&]() {
    EXPECT_TRUE(client->sync_getPtpTcEnabled());
  };

  verifyAcrossWarmBoots(setup, verify, []() {}, verifyPostWarmboot);
}

TEST_F(AgentHwPtpTcTest, VerifyPtpTcToggle) {
  AgentEnsemble* ensemble = getAgentEnsemble();
  auto l3Asics = ensemble->getL3Asics();
  auto switchId = getSw()
                      ->getScopeResolver()
                      ->scope(ensemble->masterLogicalPortIds())
                      .switchId();
  auto client = ensemble->getHwAgentTestClient(switchId);
  XLOG(DBG2) << "starting VerifyPtpTcToggle test on switchId=" << switchId;

  auto setup = [=, this]() {
    auto cfg = initialConfig(*ensemble);
    applyNewConfig(cfg);
  };
  auto enabled = false;
  auto verify = [&]() { EXPECT_EQ(enabled, client->sync_getPtpTcEnabled()); };

  setup();

  for (int i = 0; i < kLoopCount; ++i) {
    enabled = !enabled; // toggle

    setPtpTc(enabled);
    verify();
  }
}

} // namespace facebook::fboss
