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

} // namespace facebook::fboss::utility
