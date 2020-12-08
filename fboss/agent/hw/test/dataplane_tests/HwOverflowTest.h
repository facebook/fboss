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
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/HwTestProdConfigUtils.h"

#include "fboss/agent/hw/test/dataplane_tests/HwProdInvariantHelper.h"

namespace facebook::fboss {

class HwOverflowTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg =
        utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
    utility::addProdFeaturesToConfig(cfg, getHwSwitch());
    return cfg;
  }
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    prodInvariants_ = std::make_unique<HwProdInvariantHelper>(
        getHwSwitchEnsemble(), initialConfig());
    prodInvariants_->setupEcmp();
  }
  void verifyInvariants() {
    prodInvariants_->verifyInvariants();
  }
  HwSwitchEnsemble::Features featuresDesired() const override {
    return HwProdInvariantHelper::featuresDesired();
  }

 private:
  std::unique_ptr<HwProdInvariantHelper> prodInvariants_;
};
} // namespace facebook::fboss
