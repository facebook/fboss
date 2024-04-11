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
  } else if (flexMode == FlexPortMode::ONEX400G) {
    facebook::fboss::utility::assertSINGLEMode(
        hw,
        cfg::PortSpeed::FOURHUNDREDG,
        cfg::PortSpeed::HUNDREDG,
        allPortsinGroup);
  } else {
    throw FbossError("invalid FlexConfig Mode");
  }
}

cfg::PortSpeed getPortSpeed(FlexPortMode flexMode) {
  if (flexMode == FlexPortMode::FOURX10G) {
    return cfg::PortSpeed::XG;
  } else if (flexMode == FlexPortMode::FOURX25G) {
    return cfg::PortSpeed::TWENTYFIVEG;
  } else if (flexMode == FlexPortMode::ONEX40G) {
    return cfg::PortSpeed::FORTYG;
  } else if (flexMode == FlexPortMode::TWOX50G) {
    return cfg::PortSpeed::FIFTYG;
  } else if (flexMode == FlexPortMode::ONEX100G) {
    return cfg::PortSpeed::HUNDREDG;
  } else if (flexMode == FlexPortMode::ONEX400G) {
    return cfg::PortSpeed::FOURHUNDREDG;
  } else {
    throw FbossError("invalid FlexConfig Mode");
  }
}

} // unnamed namespace

class HwFlexPortTest : public HwTest {
 public:
  void flexPortApplyConfigTest(FlexPortMode flexMode, std::string configName) {
    auto modes = getHwSwitchEnsemble()->getSupportedFlexPortModes();
    if (find(modes.begin(), modes.end(), flexMode) == modes.end()) {
      XLOG(DBG2) << "Skipping: unsupported flexport mode: " << configName;
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }

    std::vector<PortID> allPortsinGroup;
    int index = 0;

    for (; index < masterLogicalPortIds().size(); index++) {
      allPortsinGroup = utility::getAllPortsInGroup(
          getHwSwitch()->getPlatform()->getPlatformMapping(),
          masterLogicalPortIds()[index]);
      if (utility::portsExistsInPortGroup(
              getHwSwitch()->getPlatform(),
              allPortsinGroup,
              getPortSpeed(flexMode)))
        break;
    }

    if (index == masterLogicalPortIds().size()) {
      XLOG(DBG2)
          << "Skipping: No ports found with matching profile for flexport mode: "
          << configName;
#if defined(GTEST_SKIP)
      GTEST_SKIP();
#endif
      return;
    }

    PortID masterLogicalPortId = masterLogicalPortIds()[index];

    auto setup = [this, &allPortsinGroup, flexMode, masterLogicalPortId]() {
      auto cfg = utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortId);
      facebook::fboss::utility::cleanPortConfig(&cfg, allPortsinGroup);
      applyNewConfig(cfg);

      cfg = utility::oneL3IntfNPortConfig(
          getHwSwitch()->getPlatform()->getPlatformMapping(),
          getHwSwitch()->getPlatform()->getAsic(),
          allPortsinGroup,
          getHwSwitch()->getPlatform()->supportsAddRemovePort());
      facebook::fboss::utility::cleanPortConfig(&cfg, allPortsinGroup);

      // Update all ports in the same group based on the flexMode
      utility::updateFlexConfig(
          &cfg, flexMode, allPortsinGroup, getHwSwitch()->getPlatform());
      applyNewConfig(cfg);
    };

    auto verify = [this, &allPortsinGroup, flexMode, masterLogicalPortId]() {
      utility::assertPortStatus(getHwSwitch(), masterLogicalPortId);
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

TEST_F(HwFlexPortTest, FlexPortONEX400G) {
  flexPortApplyConfigTest(FlexPortMode::ONEX400G, "ONEX400G");
}
