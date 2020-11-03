// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/HwSwitchStats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestSwitchUtils.h"

namespace facebook::fboss {

class HwParityErrorTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::TAM_NOTIFY};
  }
};

TEST_F(HwParityErrorTest, verifyParityError) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    auto stats = getHwSwitchEnsemble()->getHwSwitch()->getSwitchStats();
    EXPECT_EQ(stats->getCorrParityErrorCount(), 0);
    utility::generateParityError(getHwSwitchEnsemble());
    utility::verifyParityError(getHwSwitchEnsemble());
  };
  setup();
  verify();
}

} // namespace facebook::fboss
