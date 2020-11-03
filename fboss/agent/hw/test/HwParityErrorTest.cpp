// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTest.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestSwitchUtils.h"

namespace facebook::fboss {

class HwParityErrorTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::TAM_NOTIFY};
  }
};

TEST_F(HwParityErrorTest, verifyParityError) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    utility::generateParityError(getHwSwitchEnsemble());
    utility::verifyParityError(getHwSwitchEnsemble());
  };
  setup();
  verify();
}

} // namespace facebook::fboss
