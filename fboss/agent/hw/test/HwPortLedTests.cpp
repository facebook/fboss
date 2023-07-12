// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/lib/CommonUtils.h"

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
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(utility::verifyLedStatus(
          getHwSwitchEnsemble(), portID, true /* up */));
    });
    bringDownPort(portID);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(utility::verifyLedStatus(
          getHwSwitchEnsemble(), portID, false /* down */));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPortLedTest, TestLedFromSwitchState) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    auto portID = masterLogicalPortIds()[0];
    bringUpPort(portID);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(utility::verifyLedStatus(
          getHwSwitchEnsemble(), portID, true /* up */));
    });
    auto state = getProgrammedState();
    auto newState = state->clone();
    auto port = newState->getPorts()->getNode(portID);
    auto newPort = port->modify(&newState);
    newPort->setLedPortExternalState(PortLedExternalState::EXTERNAL_FORCE_OFF);
    applyNewState(newState);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(utility::verifyLedStatus(
          getHwSwitchEnsemble(), portID, false /* down */));
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
