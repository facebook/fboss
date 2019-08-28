/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmPortUtils.h"

#include "fboss/agent/hw/bcm/BcmError.h"

namespace facebook {
namespace fboss {
namespace utility {
bool portEnabled(int unit, opennsl_port_t port) {
  int enable = -1;
  auto rv = opennsl_port_enable_get(unit, port, &enable);
  bcmCheckError(rv, "failed to get port enable status");
  CHECK(enable == 0 || enable == 1);
  return (enable == 1);
}

cfg::PortSpeed curPortSpeed(int unit, opennsl_port_t port) {
  int curSpeed;
  auto ret = opennsl_port_speed_get(unit, port, &curSpeed);
  bcmCheckError(ret, "Failed to get current speed for port");
  return cfg::PortSpeed(curSpeed);
}

void assertPort(int unit, int port, bool enabled, cfg::PortSpeed speed) {
  CHECK_EQ(enabled, portEnabled(unit, port));
  if (enabled) {
    // Only verify speed on enabled ports
    CHECK_EQ(
        static_cast<int>(speed),
        static_cast<int>(utility::curPortSpeed(unit, port)));
  }
}

void assertPortStatus(int unit, int port) {
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
  auto rv = opennsl_port_control_get(
      unit, port, opennslPortControlSampleIngressDest, &sampleDestination);
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

opennsl_gport_t getPortGport(int unit, int port) {
  opennsl_gport_t portGport;
  auto rv = opennsl_port_gport_get(unit, port, &portGport);
  facebook::fboss::bcmCheckError(rv, "failed to get gport for port");
  return portGport;
}

} // namespace utility
} // namespace fboss
} // namespace facebook
