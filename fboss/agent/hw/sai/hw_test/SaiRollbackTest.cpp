/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/hw_test/SaiRollbackTest.h"

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace facebook::fboss {

cfg::SwitchConfig SaiRollbackTest::initialConfig() const {
  auto cfg =
      utility::onePortPerInterfaceConfig(getHwSwitch(), masterLogicalPortIds());
  cfg.switchSettings()->l2LearningMode() = cfg::L2LearningMode::SOFTWARE;
  utility::addProdFeaturesToConfig(cfg, getHwSwitch());
  return cfg;
}

void SaiRollbackTest::SetUp() {
  HwLinkStateDependentTest::SetUp();
  prodInvariants_ = std::make_unique<HwProdInvariantHelper>(
      getHwSwitchEnsemble(), initialConfig());
  prodInvariants_->setupEcmp();
}

void SaiRollbackTest::rollback(const std::shared_ptr<SwitchState>& state) {
  static_cast<SaiSwitch*>(getHwSwitch())->rollbackInTest(state);
  getHwSwitchEnsemble()->programmedState_ = state;
  getHwSwitchEnsemble()->programmedState_->publish();
}
} // namespace facebook::fboss
