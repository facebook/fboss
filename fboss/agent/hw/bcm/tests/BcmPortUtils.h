/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include "fboss/agent/types.h"

extern "C" {
#include <bcm/port.h>
}

namespace facebook::fboss::utility {
bool portEnabled(int unit, bcm_port_t port);
cfg::PortSpeed currentPortSpeed(int unit, bcm_port_t port);
void assertPort(int unit, int port, bool enabled, cfg::PortSpeed speed);
void assertPortStatus(int unit, int port);
void assertPortLoopbackMode(int unit, PortID port, int expectedLoopbackMode);
void assertPortSampleDestination(
    int unit,
    PortID port,
    int expectedSampleDestination);
void assertPortsLoopbackMode(
    int unit,
    const std::map<PortID, int>& port2LoopbackMode);
void assertPortsSampleDestination(
    int unit,
    const std::map<PortID, int>& port2SampleDestination);

void assertPortsFECStatus(
    int unit,
    const std::map<PortID, bool>& port2FECEnabled);
bcm_gport_t getPortGport(int unit, int port);

} // namespace facebook::fboss::utility
