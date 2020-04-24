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

} // namespace facebook::fboss::utility
