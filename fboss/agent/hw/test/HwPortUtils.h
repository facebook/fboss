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
#include "fboss/agent/platforms/tests/utils/TestPlatformTypes.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {
class HwSwitch;
class Platform;
class HwSwitchEnsemble;
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

void enablePortsInPortGroup(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    const std::vector<PortID>& allPortsInGroup,
    const Platform* platform,
    const std::array<bool, 4>& enabledPortsOption);

void cleanPortConfig(
    cfg::SwitchConfig* config,
    std::vector<PortID> allPortsinGroup);

void updateFlexConfig(
    cfg::SwitchConfig* config,
    FlexPortMode flexMode,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform);

void assertQUADMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup);

void assertDUALMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup);

void assertSINGLEMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    std::vector<PortID> allPortsinGroup);

void updateFlexConfig(
    cfg::SwitchConfig* config,
    FlexPortMode flexMode,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform);

void verifyInterfaceMode(
    PortID portID,
    cfg::PortProfileID profileID,
    Platform* platform,
    const phy::ProfileSideConfig& expectedProfileConfig);

void verifyTxSettting(
    PortID portID,
    cfg::PortProfileID profileID,
    Platform* platform,
    const std::vector<phy::PinConfig>& expectedPinConfigs);

void verifyRxSettting(
    PortID portID,
    cfg::PortProfileID profileID,
    Platform* platform,
    const std::vector<phy::PinConfig>& expectedPinConfigs);

void verifyFec(
    PortID portID,
    cfg::PortProfileID profileID,
    Platform* platform,
    const phy::ProfileSideConfig& expectedProfileConfig);

void enableSixtapProgramming();

cfg::PortSpeed getSpeed(cfg::PortProfileID profile);
TransmitterTechnology getMediaType(cfg::PortProfileID profile);
cfg::PortSpeed getDefaultFabricSpeed(const cfg::AsicType& asicType);
cfg::PortSpeed getDefaultInterfaceSpeed(const cfg::AsicType& asicType);

void verifyLedStatus(HwSwitchEnsemble* ensemble, PortID port, bool up);
} // namespace utility
} // namespace facebook::fboss
