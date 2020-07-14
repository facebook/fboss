/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/logging/xlog.h>
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

DECLARE_bool(flexports);

using namespace facebook::fboss;

namespace {

void assertFlexConfig(
    HwSwitch* hw,
    FlexPortMode flexMode,
    std::vector<PortID> allPortsinGroup) {
  if (flexMode == FlexPortMode::FOURX10G) {
    facebook::fboss::utility::assertQUADMode(
        hw, cfg::PortSpeed::XG, allPortsinGroup);
  } else if (flexMode == FlexPortMode::FOURX25G) {
    facebook::fboss::utility::assertQUADMode(
        hw, cfg::PortSpeed::TWENTYFIVEG, allPortsinGroup);
  } else if (flexMode == FlexPortMode::ONEX40G) {
    facebook::fboss::utility::assertSINGLEMode(
        hw, cfg::PortSpeed::FORTYG, cfg::PortSpeed::XG, allPortsinGroup);
  } else if (flexMode == FlexPortMode::TWOX50G) {
    facebook::fboss::utility::assertDUALMode(
        hw,
        cfg::PortSpeed::FIFTYG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup);
  } else if (flexMode == FlexPortMode::ONEX100G) {
    facebook::fboss::utility::assertSINGLEMode(
        hw,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup);
  } else {
    throw FbossError("invalid FlexConfig Mode");
  }
}

} // unnamed namespace

class HwFlexPortTest : public HwTest {
 public:
  void flexPortApplyConfigTest(FlexPortMode flexMode, std::string configName) {
    if (!FLAGS_flexports) {
      XLOG(INFO) << "Skipping flexport tests on a non flexport enabled setup";
      return;
    }
    auto modes = getHwSwitchEnsemble()->getSupportedFlexPortModes();
    if (find(modes.begin(), modes.end(), flexMode) == modes.end()) {
      XLOG(INFO) << "Skipping flexport mode: " << configName;
      return;
    }
    auto allPortsinGroup = getAllPortsInGroup(masterLogicalPortIds()[0]);

    auto setup = [this, &allPortsinGroup, flexMode]() {
      auto cfg =
          utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
      facebook::fboss::utility::cleanPortConfig(&cfg, allPortsinGroup);
      applyNewConfig(cfg);

      cfg = utility::oneL3IntfNPortConfig(getHwSwitch(), allPortsinGroup);
      facebook::fboss::utility::cleanPortConfig(&cfg, allPortsinGroup);

      auto chip = utility::getAsicChipFromPortID(
          getHwSwitch(), masterLogicalPortIds()[0]);
      for (auto portId : masterLogicalPortIds()) {
        if (utility::findCfgPortIf(cfg, portId) != cfg.ports_ref()->end() &&
            utility::getAsicChipFromPortID(getHwSwitch(), portId) == chip) {
          utility::updateFlexConfig(
              &cfg,
              flexMode,
              getAllPortsInGroup(portId),
              getHwSwitch()->getPlatform());
          if (portId != masterLogicalPortIds()[0]) {
            utility::findCfgPortIf(cfg, portId)->state_ref() =
                cfg::PortState::DISABLED;
          }
        }
      }
      applyNewConfig(cfg);
    };

    auto verify = [this, &allPortsinGroup, flexMode]() {
      utility::assertPortStatus(
          getHwSwitch(), PortID(masterLogicalPortIds()[0]));
      assertFlexConfig(getHwSwitch(), flexMode, allPortsinGroup);
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwFlexPortTest, FlexPortTWOX50G) {
  flexPortApplyConfigTest(FlexPortMode::TWOX50G, "TWOX50G");
}

TEST_F(HwFlexPortTest, FlexPortFOURX10G) {
  flexPortApplyConfigTest(FlexPortMode::FOURX10G, "FOURX10G");
}

TEST_F(HwFlexPortTest, FlexPortFOURX25G) {
  flexPortApplyConfigTest(FlexPortMode::FOURX25G, "FOURX25G");
}

TEST_F(HwFlexPortTest, FlexPortONEX40G) {
  flexPortApplyConfigTest(FlexPortMode::ONEX40G, "ONEX40G");
}

TEST_F(HwFlexPortTest, FlexPortONEX100G) {
  flexPortApplyConfigTest(FlexPortMode::ONEX100G, "ONEX100G");
}
