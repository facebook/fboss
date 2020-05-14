/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwPortUtils.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

extern "C" {
#include <bcm/port.h>
}

namespace {
bcm_port_t getBcmPort(facebook::fboss::PortID port) {
  return static_cast<bcm_port_t>(port);
}

int getUnit(const facebook::fboss::HwSwitch* hw) {
  CHECK(hw);
  return static_cast<const facebook::fboss::BcmSwitch*>(hw)->getUnit();
}

void updatePortProfileIDAndName(
    facebook::fboss::cfg::SwitchConfig* config,
    int portIndex,
    facebook::fboss::PortID portID,
    const facebook::fboss::Platform* platform) {
  auto platformPort = platform->getPlatformPort(portID);
  if (auto entry = platformPort->getPlatformPortEntry()) {
    config->ports_ref()[portIndex].name_ref() =
        *entry->mapping_ref()->name_ref();
    *config->ports[portIndex].profileID_ref() =
        platformPort->getProfileIDBySpeed(
            *config->ports[portIndex].speed_ref());
  } else {
    throw facebook::fboss::FbossError(
        "Port:", portID, " doesn't have PlatformPortEntry");
  }
}
} // namespace

namespace facebook::fboss::utility {
bool portEnabled(const HwSwitch* hw, PortID port) {
  int enable = -1;
  auto rv = bcm_port_enable_get(getUnit(hw), getBcmPort(port), &enable);
  bcmCheckError(rv, "failed to get port enable status");
  CHECK(enable == 0 || enable == 1);
  return (enable == 1);
}

cfg::PortSpeed curPortSpeed(const HwSwitch* hw, PortID port) {
  int curSpeed;
  auto ret = bcm_port_speed_get(getUnit(hw), getBcmPort(port), &curSpeed);
  bcmCheckError(ret, "Failed to get current speed for port");
  return cfg::PortSpeed(curSpeed);
}

void assertPort(
    const HwSwitch* hw,
    PortID port,
    bool enabled,
    cfg::PortSpeed speed) {
  CHECK_EQ(enabled, portEnabled(hw, port));
  if (enabled) {
    // Only verify speed on enabled ports
    CHECK_EQ(
        static_cast<int>(speed),
        static_cast<int>(utility::curPortSpeed(hw, port)));
  }
}

void assertPortStatus(const HwSwitch* hw, PortID port) {
  CHECK(portEnabled(hw, port));
}

void assertPortsLoopbackMode(
    const HwSwitch* hw,
    const std::map<PortID, int>& port2LoopbackMode) {
  for (auto portAndLoopBackMode : port2LoopbackMode) {
    assertPortLoopbackMode(
        hw, portAndLoopBackMode.first, portAndLoopBackMode.second);
  }
}

void assertPortSampleDestination(
    const HwSwitch* hw,
    PortID port,
    int expectedSampleDestination) {
  int sampleDestination;
  auto rv = bcm_port_control_get(
      getUnit(hw),
      getBcmPort(port),
      bcmPortControlSampleIngressDest,
      &sampleDestination);
  bcmCheckError(rv, "Failed to get sample destination for port:", port);
  CHECK_EQ(expectedSampleDestination, sampleDestination);
}

void assertPortsSampleDestination(
    const HwSwitch* hw,
    const std::map<PortID, int>& port2SampleDestination) {
  for (auto portAndSampleDestination : port2SampleDestination) {
    assertPortSampleDestination(
        hw, portAndSampleDestination.first, portAndSampleDestination.second);
  }
}

void assertPortLoopbackMode(
    const HwSwitch* hw,
    PortID port,
    int expectedLoopbackMode) {
  int loopbackMode;
  auto rv = bcm_port_loopback_get(getUnit(hw), getBcmPort(port), &loopbackMode);
  bcmCheckError(rv, "Failed to get loopback mode for port:", port);
  CHECK_EQ(expectedLoopbackMode, loopbackMode);
}

void enableOneLane(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  auto portItr = allPortsinGroup.begin();
  bool firstLane = true;
  int portIndex = 0;
  for (; portItr != allPortsinGroup.end(); portItr++, portIndex++) {
    *config->ports[portIndex].state_ref() =
        firstLane ? cfg::PortState::ENABLED : cfg::PortState::DISABLED;
    *config->ports[portIndex].speed_ref() =
        firstLane ? enabledLaneSpeed : disabledLaneSpeed;
    updatePortProfileIDAndName(config, portIndex, *portItr, platform);
    firstLane = false;
  }
}

void enableAllLanes(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  auto portItr = allPortsinGroup.begin();
  int portIndex = 0;
  for (; portItr != allPortsinGroup.end(); portItr++, portIndex++) {
    *config->ports[portIndex].speed_ref() = enabledLaneSpeed;
    *config->ports[portIndex].state_ref() = cfg::PortState::ENABLED;
    updatePortProfileIDAndName(config, portIndex, *portItr, platform);
  }
}

void enableTwoLanes(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  auto portItr = allPortsinGroup.begin();
  bool oddLane;
  int portIndex = 0;
  for (; portItr != allPortsinGroup.end(); portItr++, portIndex++) {
    oddLane = (*portItr - allPortsinGroup.front()) % 2 == 0 ? true : false;
    *config->ports[portIndex].state_ref() =
        oddLane ? cfg::PortState::ENABLED : cfg::PortState::DISABLED;
    *config->ports[portIndex].speed_ref() =
        oddLane ? enabledLaneSpeed : disabledLaneSpeed;
    updatePortProfileIDAndName(config, portIndex, *portItr, platform);
  }
}

void assertQUADMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    utility::assertPort(hw, *portItr, true, enabledLaneSpeed);
  }
}

void assertDUALMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  bool odd_lane;
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    odd_lane = (*portItr - allPortsinGroup.front()) % 2 == 0 ? true : false;
    bool enabled = odd_lane ? true : false;
    auto speed = odd_lane ? enabledLaneSpeed : disabledLaneSpeed;
    utility::assertPort(hw, *portItr, enabled, speed);
  }
}

void assertSINGLEMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    bool enabled = *portItr == allPortsinGroup.front() ? true : false;
    auto speed = enabled ? enabledLaneSpeed : disabledLaneSpeed;
    utility::assertPort(hw, *portItr, enabled, speed);
  }
}

} // namespace facebook::fboss::utility
