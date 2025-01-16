/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace facebook::fboss {

class AgentDiagShellStressTest : public AgentHwTest {
 protected:
  void runDiagCmds() {
    for (auto [switchId, asic] : getAsics()) {
      switch (asic->getAsicType()) {
        case cfg::AsicType::ASIC_TYPE_FAKE:
        case cfg::AsicType::ASIC_TYPE_MOCK:
        case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
        case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
        case cfg::AsicType::ASIC_TYPE_JERICHO2:
        case cfg::AsicType::ASIC_TYPE_JERICHO3:
        case cfg::AsicType::ASIC_TYPE_RAMON:
        case cfg::AsicType::ASIC_TYPE_RAMON3:
        case cfg::AsicType::ASIC_TYPE_CHENAB:
          // No diag shell to test for these ASICs
          break;
        case cfg::AsicType::ASIC_TYPE_EBRO:
        case cfg::AsicType::ASIC_TYPE_GARONNE:
        case cfg::AsicType::ASIC_TYPE_YUBA:
          runTajoDiagCmds(switchId);
          break;
        case cfg::AsicType::ASIC_TYPE_TRIDENT2:
        case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
        case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
        case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
        case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
          runBcmDiagCmds(switchId);
      }
    }
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    // TODO: introduce diag shell feature
    return {};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // check diag shell with fabric ports enabled
    FLAGS_hide_fabric_ports = false;
  }

 private:
  static auto constexpr kNumDiagCmds = 500;
  static auto constexpr kNumRestarts = 20;

  std::string makeDiagCmd(const std::string& cmd) const {
    return cmd + "\n";
  }

  void runBcmDiagCmds(SwitchID id) {
    std::string out;
    auto ensemble = getAgentEnsemble();
    // Start with an empty command for initialization
    ensemble->runDiagCommand(makeDiagCmd(""), out, id);
    for (int i = 0; i < kNumDiagCmds; i++) {
      // TODO: Change with more meaningful command
      ensemble->runDiagCommand(makeDiagCmd("ps"), out, id);
    }
    ensemble->runDiagCommand(makeDiagCmd("quit"), out, id);
    for (int i = 0; i < kNumRestarts; i++) {
      ensemble->runDiagCommand(makeDiagCmd(""), out, id);
      // TODO: Change with more meaningful command
      ensemble->runDiagCommand(makeDiagCmd("ps"), out, id);
      ensemble->runDiagCommand(makeDiagCmd("quit"), out, id);
    }
    // TODO: Validate some output.
    std::ignore = out;
  }

  void runTajoDiagCmds(SwitchID id) {
    std::string out;
    auto ensemble = getAgentEnsemble();
    ensemble->runDiagCommand(makeDiagCmd(""), out, id);
    ensemble->runDiagCommand(makeDiagCmd("from cli import sai_cli"), out, id);
    for (int i = 0; i < kNumDiagCmds; i++) {
      // TODO: Change with more meaningful command for stress testing
      ensemble->runDiagCommand(makeDiagCmd("print('hello')"), out, id);
    }
    ensemble->runDiagCommand(makeDiagCmd("quit"), out, id);
    for (int i = 0; i < kNumRestarts; i++) {
      ensemble->runDiagCommand(makeDiagCmd(""), out, id);
      ensemble->runDiagCommand(makeDiagCmd("from cli import sai_cli"), out, id);
      // TODO: Change with more meaningful command for stress testing
      ensemble->runDiagCommand(makeDiagCmd("print('hello')"), out, id);
      ensemble->runDiagCommand(makeDiagCmd("quit"), out, id);
    }
    // TODO: Validate some output.
    std::ignore = out;
  }
};

TEST_F(AgentDiagShellStressTest, stressDiagCmds) {
  auto setup = []() {};
  auto verify = [=, this]() { runDiagCmds(); };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
