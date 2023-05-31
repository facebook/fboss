// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"

namespace facebook::fboss {

class HwPortLedTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto lbMode = getPlatform()->getAsic()->desiredLoopbackModes();
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), {masterLogicalPortIds()[0]}, lbMode);
  }
};

TEST_F(HwPortLedTest, TestLed) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    auto portID = masterLogicalPortIds()[0];
    bringUpPort(portID);
    utility::verifyLedStatus(getHwSwitchEnsemble(), portID, true /* up */);
    bringDownPort(portID);
    utility::verifyLedStatus(getHwSwitchEnsemble(), portID, false /* down */);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
