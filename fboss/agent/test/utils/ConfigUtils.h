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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/switch_asics/Jericho2Asic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/hw/switch_asics/Ramon3Asic.h"
#include "fboss/agent/hw/switch_asics/RamonAsic.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>
#include <vector>

DECLARE_bool(nodeZ);

namespace facebook::fboss {
class PortMap;
class MultiSwitchPortMap;
class SwSwitch;
class TestEnsembleIf;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

/*
 * Use vlan 2000, as the base vlan for ports in configs generated here.
 * Anything except 0, 1 would actually work fine. 0 because
 * its reserved, and 1 because BRCM uses that as default VLAN.
 * So for example if we use VLAN 1, BRCM will also add cpu port to
 * that vlan along with our configured ports. This causes unnecessary
 * confusion for our tests.
 */
auto constexpr kBaseVlanId = 2000;
/*
 * Default VLAN
 */
auto constexpr kDefaultVlanId = 4094;
auto constexpr kDownlinkBaseVlanId = 2000;
auto constexpr kUplinkBaseVlanId = 4000;

const std::map<cfg::PortType, cfg::PortLoopbackMode>& kDefaultLoopbackMap();

folly::MacAddress kLocalCpuMac();

folly::MacAddress kLocalCpuMac();

std::string getLocalCpuMacStr();

bool isEnabledPortWithSubnet(
    const cfg::Port& port,
    const cfg::SwitchConfig& config);

std::vector<std::string> getLoopbackIps(SwitchID switchId);

cfg::DsfNode dsfNodeConfig(
    const HwAsic& myAsic,
    int64_t otherSwitchId = 4,
    std::optional<int> systemPortMin = std::nullopt,
    std::optional<int> systemPortMax = std::nullopt,
    const std::optional<PlatformType> platformType = std::nullopt);

std::vector<cfg::Port>::iterator findCfgPort(
    cfg::SwitchConfig& cfg,
    PortID portID);
std::vector<cfg::Port>::iterator findCfgPortIf(
    cfg::SwitchConfig& cfg,
    PortID portID);

std::unordered_map<PortID, cfg::PortProfileID> getSafeProfileIDs(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::map<PortID, std::vector<PortID>>&
        controllingPortToSubsidaryPorts,
    bool supportsAddRemovePort,
    std::optional<std::vector<PortID>> masterLogicalPortIds = std::nullopt);

cfg::SwitchConfig onePortPerInterfaceConfig(
    const SwSwitch* swSwitch,
    const std::vector<PortID>& ports,
    bool interfaceHasSubnet = true,
    bool setInterfaceMac = true,
    int baseIntfId = kBaseVlanId,
    bool enableFabricPorts = false);

cfg::SwitchConfig onePortPerInterfaceConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::vector<PortID>& ports,
    bool supportsAddRemovePort,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap,
    bool interfaceHasSubnet = true,
    bool setInterfaceMac = true,
    int baseIntfId = kBaseVlanId,
    bool enableFabricPorts = false,
    const std::optional<std::map<SwitchID, cfg::SwitchInfo>>&
        switchIdToSwitchInfo = std::nullopt,
    const std::optional<std::map<SwitchID, const HwAsic*>>& hwAsicTable =
        std::nullopt,
    const std::optional<PlatformType> platformType = std::nullopt);

cfg::SwitchConfig onePortPerInterfaceConfig(
    const TestEnsembleIf* ensemble,
    const std::vector<PortID>& ports,
    bool interfaceHasSubnet = true,
    bool setInterfaceMac = true,
    int baseIntfId = kBaseVlanId,
    bool enableFabricPorts = false);

cfg::SwitchConfig
oneL3IntfTwoPortConfig(const SwSwitch* sw, PortID port1, PortID port2);

cfg::SwitchConfig oneL3IntfTwoPortConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    PortID port1,
    PortID port2,
    bool supportsAddRemovePort,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap());

cfg::SwitchConfig oneL3IntfNPortConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::vector<PortID>& ports,
    bool supportsAddRemovePort,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap(),
    bool interfaceHasSubnet = true,
    int baseVlanId = kBaseVlanId,
    bool optimizePortProfile = true,
    bool setInterfaceMac = true);

cfg::SwitchConfig multiplePortsPerIntfConfig(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::vector<PortID>& ports,
    bool supportsAddRemovePort,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap(),
    bool interfaceHasSubnet = true,
    bool setInterfaceMac = true,
    const int baseVlanId = kBaseVlanId,
    const int portsPerVlan = 1,
    bool enableFabricPorts = false,
    const std::optional<std::map<SwitchID, cfg::SwitchInfo>>&
        switchIdToSwitchInfo = std::nullopt,
    const std::optional<std::map<SwitchID, const HwAsic*>>& hwAsicTable =
        std::nullopt,
    const std::optional<PlatformType> platformType = std::nullopt);

cfg::SwitchConfig genPortVlanCfg(
    const PlatformMapping* platformMapping,
    const HwAsic* asic,
    const std::vector<PortID>& ports,
    const std::map<PortID, VlanID>& port2vlan,
    const std::vector<VlanID>& vlans,
    const std::map<cfg::PortType, cfg::PortLoopbackMode> lbModeMap,
    bool supportsAddRemovePort,
    bool optimizePortProfile = true,
    bool enableFabricPorts = false,
    const std::optional<std::map<SwitchID, cfg::SwitchInfo>>&
        switchIdToSwitchInfo = std::nullopt,
    const std::optional<std::map<SwitchID, const HwAsic*>>& hwAsicTable =
        std::nullopt,
    const std::optional<PlatformType> platformType = std::nullopt);

void populateSwitchInfo(
    cfg::SwitchConfig& config,
    const std::map<SwitchID, cfg::SwitchInfo>& switchIdToSwitchInfo,
    const std::map<SwitchID, const HwAsic*>& hwAsicTable,
    const std::optional<PlatformType> platformType = std::nullopt);

cfg::SwitchConfig twoL3IntfConfig(
    SwSwitch* swSwitch,
    PortID port1,
    PortID port2,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap());
cfg::SwitchConfig twoL3IntfConfig(
    const PlatformMapping* platformMapping,
    const std::vector<const HwAsic*>& asics,
    bool supportsAddRemovePort,
    PortID port1,
    PortID port2,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap());
void addMatcher(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    const cfg::MatchAction& matchAction);
void delMatcher(cfg::SwitchConfig* config, const std::string& matcherName);

/*
 * Currently we rely on port max speed to set the PortProfileID in the default
 * port config. This can be expensive as if the Hardware comes up with ports
 * using speeds different than max speed, the system will try to reconfigure
 * the port group or the port speed. But most of our tests don't really care
 * about which speed we use.
 * Therefore, introducing this static PortToDefaultProfileIDMap so that
 * when HwTest::SetUp() finish initializng the HwSwitchEnsemble, we can use
 * the SwState to collect the port and current profile id and then update
 * this map. Using this SwState, which represents the state of the Hardware w/o
 * any config applied yet, the port config can truely represent the default
 * state of the Hardware.
 */
std::unordered_map<PortID, cfg::PortProfileID>& getPortToDefaultProfileIDMap();

bool isRswPlatform(PlatformType type);

/*
 * Functions to get uplinks and downlinks return a pair of vectors, which is a
 * lot to write out, so we define a simple type that's descriptive and saves a
 * few keystrokes.
 */
typedef std::pair<std::vector<PortID>, std::vector<PortID>> UplinkDownlinkPair;

UplinkDownlinkPair getRswUplinkDownlinkPorts(
    const cfg::SwitchConfig& config,
    const int ecmpWidth);

UplinkDownlinkPair getRtswUplinkDownlinkPorts(
    const cfg::SwitchConfig& config,
    const int ecmpWidth);

UplinkDownlinkPair getAllUplinkDownlinkPorts(
    PlatformType platformType,
    const cfg::SwitchConfig& config,
    const int ecmpWidth = 4,
    const bool mmu_lossless = false);

void configurePortGroup(
    const PlatformMapping* platformMapping,
    bool supportsAddRemovePort,
    cfg::SwitchConfig& config,
    cfg::PortSpeed speed,
    std::vector<PortID> allPortsInGroup);

std::vector<PortID> getAllPortsInGroup(
    const PlatformMapping* platformMapping,
    PortID portID);

void removeSubsumedPorts(
    cfg::SwitchConfig& config,
    const cfg::PlatformPortConfig& profile,
    bool supportsAddRemovePort);

std::map<int, std::vector<uint8_t>> getOlympicQosMaps(
    const cfg::SwitchConfig& config);

bool checkConfigHasAclEntry(
    const cfg::SwitchConfig& config,
    std::string aclName);

void configurePortProfile(
    const PlatformMapping* platformMapping,
    bool supportsAddRemovePort,
    cfg::SwitchConfig& config,
    cfg::PortProfileID profileID,
    std::vector<PortID> allPortsInGroup,
    PortID controllingPortID);
void setupMultipleEgressPoolAndQueueConfigs(
    cfg::SwitchConfig& config,
    const std::vector<int>& losslessQueueIds);
} // namespace facebook::fboss::utility
