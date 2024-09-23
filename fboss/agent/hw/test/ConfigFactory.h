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
#include <folly/MacAddress.h>
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/types.h"

#include <vector>

namespace facebook::fboss {
class PortMap;
class MultiSwitchPortMap;
class HwSwitchEnsemble;
class HwAsicTable;
} // namespace facebook::fboss

/*
 * This utility is to provide utils for test.
 */

namespace facebook::fboss::utility {
cfg::SwitchConfig oneL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap(),
    int baseVlanId = kBaseVlanId);
cfg::SwitchConfig oneL3IntfConfig(
    const PlatformMapping* platformMapping,
    const std::vector<const HwAsic*>& asics,
    PortID port,
    bool supportsAddRemovePort,
    int baseVlanId = kBaseVlanId);
cfg::SwitchConfig oneL3IntfNoIPAddrConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap());
cfg::SwitchConfig onePortPerInterfaceConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap(),
    bool interfaceHasSubnet = true,
    bool setInterfaceMac = true,
    int baseVlanId = kBaseVlanId,
    bool enableFabricPorts = false);

cfg::SwitchConfig twoL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port1,
    PortID port2,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap());
void updatePortSpeed(
    const PlatformMapping* platformMapping,
    bool supportsAddRemovePort,
    cfg::SwitchConfig& cfg,
    PortID port,
    cfg::PortSpeed speed);

std::string getAsicChipFromPortID(const HwSwitch* hwSwitch, PortID id);

std::vector<PortDescriptor> getUplinksForEcmp(
    const HwSwitch* hwSwitch,
    const cfg::SwitchConfig& config,
    const int uplinkCount,
    const bool mmu_lossless_mode = false);

cfg::SwitchConfig createUplinkDownlinkConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    PlatformType platformType,
    bool supportsAddRemovePort,
    const std::vector<PortID>& masterLogicalPortIds,
    uint16_t uplinksCount,
    cfg::PortSpeed uplinkPortSpeed,
    cfg::PortSpeed downlinkPortSpeed,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap(),
    bool interfaceHasSubnet = true);

cfg::SwitchConfig createRtswUplinkDownlinkConfig(
    const HwSwitch* hwSwitch,
    HwSwitchEnsemble* ensemble,
    const std::vector<PortID>& masterLogicalPortIds,
    cfg::PortSpeed portSpeed,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    std::vector<PortID>& uplinks,
    std::vector<PortID>& downlinks);

std::pair<int, int> getRetryCountAndDelay(const HwAsicTable* hwAsicTable);

void setPortToDefaultProfileIDMap(
    const std::shared_ptr<MultiSwitchPortMap>& ports,
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    bool supportsAddRemove,
    std::optional<std::vector<PortID>> masterLogicalPortIds = std::nullopt);

UplinkDownlinkPair getAllUplinkDownlinkPorts(
    const HwSwitch* hwSwitch,
    const cfg::SwitchConfig& config,
    const int ecmpWidth = 4,
    const bool mmu_lossless = false);

} // namespace facebook::fboss::utility
