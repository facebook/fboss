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

extern "C" {
#include <bcm/port.h>
}

namespace {
bcm_port_t getBcmPort(facebook::fboss::PortID port) {
  return static_cast<bcm_port_t>(port);
}
} // namespace

namespace facebook::fboss::utility {
bool portEnabled(int unit, PortID port) {
  int enable = -1;
  auto rv = bcm_port_enable_get(unit, getBcmPort(port), &enable);
  bcmCheckError(rv, "failed to get port enable status");
  CHECK(enable == 0 || enable == 1);
  return (enable == 1);
}

cfg::PortSpeed curPortSpeed(int unit, PortID port) {
  int curSpeed;
  auto ret = bcm_port_speed_get(unit, getBcmPort(port), &curSpeed);
  bcmCheckError(ret, "Failed to get current speed for port");
  return cfg::PortSpeed(curSpeed);
}

void assertPort(int unit, PortID port, bool enabled, cfg::PortSpeed speed) {
  CHECK_EQ(enabled, portEnabled(unit, port));
  if (enabled) {
    // Only verify speed on enabled ports
    CHECK_EQ(
        static_cast<int>(speed),
        static_cast<int>(utility::curPortSpeed(unit, port)));
  }
}

void assertPortStatus(int unit, PortID port) {
  CHECK(portEnabled(unit, port));
}

void assertPortsLoopbackMode(
    int unit,
    const std::map<PortID, int>& port2LoopbackMode) {
  for (auto portAndLoopBackMode : port2LoopbackMode) {
    assertPortLoopbackMode(
        unit, portAndLoopBackMode.first, portAndLoopBackMode.second);
  }
}

void assertPortSampleDestination(
    int unit,
    PortID port,
    int expectedSampleDestination) {
  int sampleDestination;
  auto rv = bcm_port_control_get(
      unit,
      getBcmPort(port),
      bcmPortControlSampleIngressDest,
      &sampleDestination);
  bcmCheckError(rv, "Failed to get sample destination for port:", port);
  CHECK_EQ(expectedSampleDestination, sampleDestination);
}

void assertPortsSampleDestination(
    int unit,
    const std::map<PortID, int>& port2SampleDestination) {
  for (auto portAndSampleDestination : port2SampleDestination) {
    assertPortSampleDestination(
        unit, portAndSampleDestination.first, portAndSampleDestination.second);
  }
}

void assertPortLoopbackMode(int unit, PortID port, int expectedLoopbackMode) {
  int loopbackMode;
  auto rv = bcm_port_loopback_get(unit, getBcmPort(port), &loopbackMode);
  bcmCheckError(rv, "Failed to get loopback mode for port:", port);
  CHECK_EQ(expectedLoopbackMode, loopbackMode);
}

} // namespace facebook::fboss::utility
