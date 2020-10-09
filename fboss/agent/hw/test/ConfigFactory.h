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
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>

#include <vector>

/*
 * This utility is to provide utils for test.
 */

namespace facebook::fboss::utility {

/*
 * Use vlan 1000, as the base vlan for ports in configs generated here.
 * Anything except 0, 1 would actually work fine. 0 because
 * its reserved, and 1 because BRCM uses that as default VLAN.
 * So for example if we use VLAN 1, BRCM will also add cpu port to
 * that vlan along with our configured ports. This causes unnecessary
 * confusion for our tests.
 */
auto constexpr kBaseVlanId = 1000;
/*
 * Default VLAN
 */
auto constexpr kDefaultVlanId = 4094;

auto constexpr kDownlinkBaseVlanId = 2000;

folly::MacAddress kLocalCpuMac();

cfg::SwitchConfig oneL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE);
cfg::SwitchConfig oneL3IntfNoIPAddrConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE);
cfg::SwitchConfig oneL3IntfTwoPortConfig(
    const HwSwitch* hwSwitch,
    PortID port1,
    PortID port2,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE);
cfg::SwitchConfig oneL3IntfNPortConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE,
    bool interfaceHasSubnet = true);
cfg::SwitchConfig onePortPerVlanConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE,
    bool interfaceHasSubnet = true);
cfg::SwitchConfig twoL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port1,
    PortID port2,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE);
void updatePortProfile(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& cfg,
    PortID port,
    cfg::PortProfileID profileID);
void updatePortSpeed(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& cfg,
    PortID port,
    cfg::PortSpeed speed);
std::vector<cfg::Port>::iterator findCfgPort(
    cfg::SwitchConfig& cfg,
    PortID portID);
std::vector<cfg::Port>::iterator findCfgPortIf(
    cfg::SwitchConfig& cfg,
    PortID portID);
void configurePortGroup(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& config,
    cfg::PortSpeed speed,
    std::vector<PortID> allPortsInGroup);
void configurePortProfile(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& config,
    cfg::PortProfileID profileID,
    std::vector<PortID> allPortsInGroup);
std::string getAsicChipFromPortID(const HwSwitch* hwSwitch, PortID id);

void addMatcher(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    const cfg::MatchAction& matchAction);
std::vector<PortID> getAllPortsInGroup(const HwSwitch* hwSwitch, PortID portID);

cfg::SwitchConfig createUplinkDownlinkConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds,
    uint16_t uplinksCount,
    cfg::PortSpeed uplinkPortSpeed,
    cfg::PortSpeed downlinkPortSpeed,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE,
    bool interfaceHasSubnet = true);
} // namespace facebook::fboss::utility
