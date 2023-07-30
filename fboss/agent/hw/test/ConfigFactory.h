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
#include "fboss/agent/types.h"

#include <vector>

namespace facebook::fboss {
class PortMap;
class MultiSwitchPortMap;
class HwSwitchEnsemble;
} // namespace facebook::fboss

/*
 * This utility is to provide utils for test.
 */

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

bool isEnabledPortWithSubnet(
    const cfg::Port& port,
    const cfg::SwitchConfig& config);

std::vector<std::string> getLoopbackIps(SwitchID switchId);

cfg::DsfNode dsfNodeConfig(const HwAsic& myAsic, int64_t otherSwitchId = 4);

cfg::SwitchConfig oneL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap(),
    int baseVlanId = kBaseVlanId);
cfg::SwitchConfig oneL3IntfNoIPAddrConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap());
cfg::SwitchConfig oneL3IntfTwoPortConfig(
    const HwSwitch* hwSwitch,
    PortID port1,
    PortID port2,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap());
cfg::SwitchConfig oneL3IntfNPortConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap(),
    bool interfaceHasSubnet = true,
    int baseVlanId = kBaseVlanId,
    bool optimizePortProfile = true,
    bool setInterfaceMac = true);
cfg::SwitchConfig onePortPerInterfaceConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap(),
    bool interfaceHasSubnet = true,
    bool setInterfaceMac = true,
    int baseVlanId = kBaseVlanId,
    bool enableFabricPorts = false);
cfg::SwitchConfig multiplePortsPerIntfConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap(),
    bool interfaceHasSubnet = true,
    bool setInterfaceMac = true,
    const int baseVlanId = kBaseVlanId,
    const int portsPerVlan = 1,
    bool enableFabricPorts = false);

cfg::SwitchConfig twoL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port1,
    PortID port2,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>& lbModeMap =
        kDefaultLoopbackMap());
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
    std::vector<PortID> allPortsInGroup,
    PortID controllingPortID);
std::string getAsicChipFromPortID(const HwSwitch* hwSwitch, PortID id);

void addMatcher(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    const cfg::MatchAction& matchAction);
void delMatcher(cfg::SwitchConfig* config, const std::string& matcherName);
std::vector<PortID> getAllPortsInGroup(const HwSwitch* hwSwitch, PortID portID);

std::vector<PortDescriptor> getUplinksForEcmp(
    const HwSwitch* hwSwitch,
    const cfg::SwitchConfig& config,
    const int uplinkCount,
    const bool mmu_lossless_mode = false);

cfg::SwitchConfig createUplinkDownlinkConfig(
    const HwSwitch* hwSwitch,
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
std::pair<int, int> getRetryCountAndDelay(const HwAsic* asic);

void setPortToDefaultProfileIDMap(
    const std::shared_ptr<MultiSwitchPortMap>& ports,
    const Platform* platform);

std::map<int, std::vector<uint8_t>> getOlympicQosMaps(
    const cfg::SwitchConfig& config);

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
    const HwSwitch* hwSwitch,
    const cfg::SwitchConfig& config,
    const int ecmpWidth = 4,
    const bool mmu_lossless = false);

} // namespace facebook::fboss::utility
