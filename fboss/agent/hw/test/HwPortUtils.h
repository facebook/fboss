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

namespace facebook::fboss {
class HwSwitch;
namespace utility {
bool portEnabled(const HwSwitch* hw, PortID port);
cfg::PortSpeed currentPortSpeed(const HwSwitch* hw, PortID port);
void assertPort(
    const HwSwitch* hw,
    PortID port,
    bool enabled,
    cfg::PortSpeed speed);
void assertPortStatus(const HwSwitch* hw, PortID port);
void assertPortLoopbackMode(
    const HwSwitch* hw,
    PortID port,
    int expectedLoopbackMode);
void assertPortSampleDestination(
    const HwSwitch* hw,
    PortID port,
    int expectedSampleDestination);
void assertPortsLoopbackMode(
    const HwSwitch* hw,
    const std::map<PortID, int>& port2LoopbackMode);
void assertPortsSampleDestination(
    const HwSwitch* hw,
    const std::map<PortID, int>& port2SampleDestination);
} // namespace utility
} // namespace facebook::fboss
