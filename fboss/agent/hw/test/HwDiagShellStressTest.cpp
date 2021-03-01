/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/HwSwitchStats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {

class HwDiagShellStressTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  }

  void runDiagCmds() {
    auto asic = getPlatform()->getAsic()->getAsicType();
    switch (asic) {
      case HwAsic::AsicType::ASIC_TYPE_FAKE:
      case HwAsic::AsicType::ASIC_TYPE_MOCK:
      case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
        // No diag shell to test for these ASICs
        break;
      case HwAsic::AsicType::ASIC_TYPE_TAJO:
        runTajoDiagCmds();
        break;
      case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
        runBcmDiagCmds();
    }
  }

 private:
  static auto constexpr kNumDiagCmds = 500;
  static auto constexpr kNumRestarts = 20;

  std::string makeDiagCmd(const std::string& cmd) const {
    return cmd + "\n";
  }

  void runBcmDiagCmds() {
    std::string out;
    auto ensemble = getHwSwitchEnsemble();
    // Start with an empty command for initialization
    ensemble->runDiagCommand(makeDiagCmd(""), out);
    for (int i = 0; i < kNumDiagCmds; i++) {
      // TODO: Change with more meaningful command
      ensemble->runDiagCommand(makeDiagCmd("ps"), out);
    }
    ensemble->runDiagCommand(makeDiagCmd("quit"), out);
    for (int i = 0; i < kNumRestarts; i++) {
      ensemble->runDiagCommand(makeDiagCmd(""), out);
      // TODO: Change with more meaningful command
      ensemble->runDiagCommand(makeDiagCmd("ps"), out);
      ensemble->runDiagCommand(makeDiagCmd("quit"), out);
    }
    // TODO: Validate some output.
    std::ignore = out;
  }

  void runTajoDiagCmds() {
    std::string out;
    auto ensemble = getHwSwitchEnsemble();
    ensemble->runDiagCommand(makeDiagCmd(""), out);
    ensemble->runDiagCommand(makeDiagCmd("from cli import sai_cli"), out);
    for (int i = 0; i < kNumDiagCmds; i++) {
      // TODO: Change with more meaningful command for stress testing
      ensemble->runDiagCommand(makeDiagCmd("print('hello')"), out);
    }
    ensemble->runDiagCommand(makeDiagCmd("quit"), out);
    for (int i = 0; i < kNumRestarts; i++) {
      ensemble->runDiagCommand(makeDiagCmd(""), out);
      ensemble->runDiagCommand(makeDiagCmd("from cli import sai_cli"), out);
      // TODO: Change with more meaningful command for stress testing
      ensemble->runDiagCommand(makeDiagCmd("print('hello')"), out);
      ensemble->runDiagCommand(makeDiagCmd("quit"), out);
    }
    // TODO: Validate some output.
    std::ignore = out;
  }
};

TEST_F(HwDiagShellStressTest, stressDiagCmds) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() { runDiagCmds(); };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
