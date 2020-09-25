// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestPortUtils.h"
#include "fboss/agent/state/Port.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {
class HwPortTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::oneL3IntfTwoPortConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }
};

TEST_F(HwPortTest, AssertMode) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };
  auto verify = [this]() {
    for (const auto& port : *getProgrammedState()->getPorts()) {
      if (!port->isEnabled()) {
        continue;
      }
      utility::verifyInterfaceMode(
          port->getID(), port->getProfileID(), getPlatform());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPortTest, AssertTxSetting) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };
  auto verify = [this]() {
    for (const auto& port : *getProgrammedState()->getPorts()) {
      if (!port->isEnabled()) {
        continue;
      }
      utility::verifyTxSettting(
          port->getID(), port->getProfileID(), getPlatform());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwPortTest, AssertRxSetting) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };
  auto verify = [this]() {
    for (const auto& port : *getProgrammedState()->getPorts()) {
      if (!port->isEnabled()) {
        continue;
      }
      utility::verifyRxSettting(
          port->getID(), port->getProfileID(), getPlatform());
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
