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
#include "fboss/agent/hw/bcm/tests/BcmPortUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

DECLARE_bool(flexports);

using namespace facebook::fboss;

namespace {

void enableOneLane(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  auto portItr = allPortsinGroup.begin();
  bool firstLane = true;
  int portIndex = 0;
  for (; portItr != allPortsinGroup.end(); portItr++, portIndex++) {
    config->ports[portIndex].state =
        firstLane ? cfg::PortState::ENABLED : cfg::PortState::DISABLED;
    config->ports[portIndex].speed =
        firstLane ? enabledLaneSpeed : disabledLaneSpeed;
    firstLane = false;
  }
}

void enableAllLanes(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  auto portItr = allPortsinGroup.begin();
  int portIndex = 0;
  for (; portItr != allPortsinGroup.end(); portItr++, portIndex++) {
    config->ports[portIndex].speed = enabledLaneSpeed;
    config->ports[portIndex].state = cfg::PortState::ENABLED;
  }
}

void enableTwoLanes(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  auto portItr = allPortsinGroup.begin();
  bool oddLane;
  int portIndex = 0;
  for (; portItr != allPortsinGroup.end(); portItr++, portIndex++) {
    oddLane = (*portItr - allPortsinGroup.front()) % 2 == 0 ? true : false;
    config->ports[portIndex].state =
        oddLane ? cfg::PortState::ENABLED : cfg::PortState::DISABLED;
    config->ports[portIndex].speed =
        oddLane ? enabledLaneSpeed : disabledLaneSpeed;
  }
}

void updateFlexConfig(
    cfg::SwitchConfig* config,
    FlexPortMode flexMode,
    std::vector<PortID> allPortsinGroup) {
  if (flexMode == FlexPortMode::FOURX10G) {
    enableAllLanes(config, cfg::PortSpeed::XG, allPortsinGroup);
  } else if (flexMode == FlexPortMode::FOURX25G) {
    enableAllLanes(config, cfg::PortSpeed::TWENTYFIVEG, allPortsinGroup);
  } else if (flexMode == FlexPortMode::ONEX40G) {
    enableOneLane(
        config, cfg::PortSpeed::FORTYG, cfg::PortSpeed::XG, allPortsinGroup);
  } else if (flexMode == FlexPortMode::TWOX50G) {
    enableTwoLanes(
        config,
        cfg::PortSpeed::FIFTYG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup);
  } else if (flexMode == FlexPortMode::ONEX100G) {
    enableOneLane(
        config,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup);
  } else {
    throw std::runtime_error("invalid FlexConfig Mode");
  }
}

void assertQUADMode(
    int unit,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    utility::assertPort(unit, *portItr, true, enabledLaneSpeed);
  }
}

void assertDUALMode(
    int unit,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  bool odd_lane;
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    odd_lane = (*portItr - allPortsinGroup.front()) % 2 == 0 ? true : false;
    bool enabled = odd_lane ? true : false;
    auto speed = odd_lane ? enabledLaneSpeed : disabledLaneSpeed;
    utility::assertPort(unit, *portItr, enabled, speed);
  }
}

void assertSINGLEMode(
    int unit,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    bool enabled = *portItr == allPortsinGroup.front() ? true : false;
    auto speed = enabled ? enabledLaneSpeed : disabledLaneSpeed;
    utility::assertPort(unit, *portItr, enabled, speed);
  }
}

void assertFlexConfig(
    int unit,
    FlexPortMode flexMode,
    std::vector<PortID> allPortsinGroup) {
  if (flexMode == FlexPortMode::FOURX10G) {
    assertQUADMode(unit, cfg::PortSpeed::XG, allPortsinGroup);
  } else if (flexMode == FlexPortMode::FOURX25G) {
    assertQUADMode(unit, cfg::PortSpeed::TWENTYFIVEG, allPortsinGroup);
  } else if (flexMode == FlexPortMode::ONEX40G) {
    assertSINGLEMode(
        unit, cfg::PortSpeed::FORTYG, cfg::PortSpeed::XG, allPortsinGroup);
  } else if (flexMode == FlexPortMode::TWOX50G) {
    assertDUALMode(
        unit,
        cfg::PortSpeed::FIFTYG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup);
  } else if (flexMode == FlexPortMode::ONEX100G) {
    assertSINGLEMode(
        unit,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup);
  } else {
    throw std::runtime_error("invalid FlexConfig Mode");
  }
}

} // unnamed namespace

class BcmFlexPortTest : public BcmTest {
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
    auto allPortsinGroup = getAllPortsinGroup(masterLogicalPortIds()[0]);

    auto setup = [this, &allPortsinGroup, flexMode]() {
      auto cfg =
          utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
      applyNewConfig(cfg);

      cfg = utility::oneL3IntfNPortConfig(getHwSwitch(), allPortsinGroup);
      updateFlexConfig(&cfg, flexMode, allPortsinGroup);
      applyNewConfig(cfg);
    };

    auto verify = [this, &allPortsinGroup, flexMode]() {
      utility::assertPortStatus(getUnit(), masterLogicalPortIds()[0]);
      assertFlexConfig(getUnit(), flexMode, allPortsinGroup);
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(BcmFlexPortTest, FlexPortTWOX50G) {
  flexPortApplyConfigTest(FlexPortMode::TWOX50G, "TWOX50G");
}

TEST_F(BcmFlexPortTest, FlexPortFOURX10G) {
  flexPortApplyConfigTest(FlexPortMode::FOURX10G, "FOURX10G");
}

TEST_F(BcmFlexPortTest, FlexPortFOURX25G) {
  flexPortApplyConfigTest(FlexPortMode::FOURX25G, "FOURX25G");
}

TEST_F(BcmFlexPortTest, FlexPortONEX40G) {
  flexPortApplyConfigTest(FlexPortMode::ONEX40G, "ONEX40G");
}

TEST_F(BcmFlexPortTest, FlexPortONEX100G) {
  flexPortApplyConfigTest(FlexPortMode::ONEX100G, "ONEX100G");
}
