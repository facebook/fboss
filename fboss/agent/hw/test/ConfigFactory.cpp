/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMode.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

namespace {

// TODO(ccpowers): remove this once we've made platformPortEntry a required
// field
cfg::PortSpeed maxPortSpeed(const HwSwitch* hwSwitch, PortID port) {
  // If Hardware can't decide the max speed, we use Platform(PlatformMapping) to
  // decide the max speed.
  try {
    if (auto maxSpeed = hwSwitch->getPortMaxSpeed(port);
        maxSpeed != cfg::PortSpeed::DEFAULT) {
      return maxSpeed;
    }
  } catch (const FbossError& e) {
    XLOG(DBG5) << "cannot access port in hwSwitch " << folly::exceptionStr(e);
  }
  return hwSwitch->getPlatform()->getPortMaxSpeed(port);
}

// Return the ID for a profile that doesn't subsume any other ports.
cfg::PortProfileID getSafeProfileID(const cfg::PlatformPortEntry& portEntry) {
  for (auto it : *portEntry.supportedProfiles_ref()) {
    auto id = it.first;
    auto cfg = it.second;
    if (!cfg.subsumedPorts_ref() || cfg.subsumedPorts_ref()->empty()) {
      return id;
    }
  }

  throw FbossError(
      "No safe profile found for port ", *portEntry.mapping_ref()->id_ref());
}

cfg::PortSpeed getPortSpeedFromProfile(
    const Platform* platform,
    cfg::PortProfileID profileID) {
  auto profile = platform->getPortProfileConfig(profileID);
  if (!profile.has_value()) {
    throw FbossError("No profile ", profileID, " found for platform");
  }
  return *profile->speed_ref();
}

cfg::Port createDefaultPortConfig(const HwSwitch* hwSwitch, PortID id) {
  cfg::Port defaultConfig;
  auto platform = hwSwitch->getPlatform();
  // Platforms that require the profile ID should all have platformPortEntries
  // generated. Other platforms can just use their max port speed until
  // we make it a required field.
  // If the platform supports removing ports, then pick a speed profile that
  // doesn't remove any ports. Otherwise just use the max speed available.
  if (auto entry = platform->getPlatformPort(id)->getPlatformPortEntry();
      entry && platform->supportsAddRemovePort()) {
    defaultConfig.name_ref() = *entry->mapping_ref()->name_ref();
    *defaultConfig.profileID_ref() = getSafeProfileID(entry.value());
    if (*defaultConfig.profileID_ref() == cfg::PortProfileID::PROFILE_DEFAULT) {
      throw FbossError(
          *entry->mapping_ref()->name_ref(), " has DEFAULT safe profile");
    }
    *defaultConfig.speed_ref() =
        getPortSpeedFromProfile(platform, *defaultConfig.profileID_ref());
  } else {
    XLOG(DBG5) << "No platformPortEntry for port " << id
               << ", defaulting to max port speed instead of safe profile";
    *defaultConfig.speed_ref() = maxPortSpeed(hwSwitch, id);
    defaultConfig.name_ref() = "eth1/" + std::to_string(id) + "/1";
  }

  *defaultConfig.logicalID_ref() = id;
  *defaultConfig.state_ref() = cfg::PortState::DISABLED;

  return defaultConfig;
}

// Fill in configs for all remaining ports that exist in the default state
cfg::SwitchConfig createDefaultConfig(const HwSwitch* hwSwitch) {
  cfg::SwitchConfig config;
  auto platform = hwSwitch->getPlatform();

  for (const auto& it : platform->getPlatformPorts()) {
    auto mapping = it.second.get_mapping();
    auto id = *mapping.id_ref();
    if (findCfgPortIf(config, PortID(id)) != config.ports_ref()->end()) {
      continue;
    }
    config.ports_ref()->push_back(
        createDefaultPortConfig(hwSwitch, PortID(id)));
  }
  return config;
}

cfg::SwitchConfig genPortVlanCfg(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    const std::map<PortID, VlanID>& port2vlan,
    const std::vector<VlanID>& vlans,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE) {
  auto config = createDefaultConfig(hwSwitch);

  // Port config
  std::map<std::string, cfg::PortProfileID> chipToProfileID;
  for (auto portID : ports) {
    if (findCfgPortIf(config, portID) == config.ports_ref()->end()) {
      config.ports_ref()->push_back(createDefaultPortConfig(hwSwitch, portID));
    }
    updatePortSpeed(*hwSwitch, config, portID, maxPortSpeed(hwSwitch, portID));
    auto portCfg = findCfgPort(config, portID);
    auto chip = getAsicChipFromPortID(hwSwitch, portID);
    chipToProfileID[chip] = *portCfg->profileID_ref();
    portCfg->maxFrameSize_ref() = 9412;
    portCfg->state_ref() = cfg::PortState::ENABLED;
    portCfg->loopbackMode_ref() = lbMode;
    portCfg->ingressVlan_ref() = port2vlan.find(portID)->second;
    portCfg->routable_ref() = true;
    portCfg->parserType_ref() = cfg::ParserType::L3;
  }

  // make ports belonging to the same asic core use the same profileID
  for (auto port : hwSwitch->getPlatform()->getPlatformPorts()) {
    auto portID = PortID(port.first);
    auto chip = getAsicChipFromPortID(hwSwitch, portID);
    if (findCfgPortIf(config, portID) != config.ports_ref()->end() &&
        chipToProfileID.find(chip) != chipToProfileID.end()) {
      auto portCfg = findCfgPort(config, portID);
      if (chipToProfileID[chip] != *portCfg->profileID_ref()) {
        updatePortProfile(*hwSwitch, config, portID, chipToProfileID[chip]);
      }
    }
  }

  // Vlan config
  for (auto vlanID : vlans) {
    cfg::Vlan vlan;
    vlan.id_ref() = vlanID;
    vlan.name_ref() = "vlan" + std::to_string(vlanID);
    vlan.routable_ref() = true;
    config.vlans_ref()->push_back(vlan);
  }

  cfg::Vlan defaultVlan;
  defaultVlan.id_ref() = kDefaultVlanId;
  defaultVlan.name_ref() = folly::sformat("vlan{}", kDefaultVlanId);
  defaultVlan.routable_ref() = true;
  config.vlans_ref()->push_back(defaultVlan);
  config.defaultVlan_ref() = kDefaultVlanId;

  // Vlan port config
  for (auto vlanPortPair : port2vlan) {
    cfg::VlanPort vlanPort;
    vlanPort.logicalPort_ref() = vlanPortPair.first;
    vlanPort.vlanID_ref() = vlanPortPair.second;
    vlanPort.spanningTreeState_ref() = cfg::SpanningTreeState::FORWARDING;
    vlanPort.emitTags_ref() = false;
    config.vlanPorts_ref()->push_back(vlanPort);
  }

  return config;
}

std::string getLocalCpuMacStr() {
  return kLocalCpuMac().toString();
}

void removePort(
    cfg::SwitchConfig& config,
    PortID port,
    bool supportsAddRemovePort) {
  auto cfgPort = findCfgPortIf(config, port);
  if (cfgPort == config.ports_ref()->end()) {
    return;
  }
  if (supportsAddRemovePort) {
    config.ports_ref()->erase(cfgPort);
    auto removed = std::remove_if(
        config.vlanPorts_ref()->begin(),
        config.vlanPorts_ref()->end(),
        [port](auto vlanPort) {
          return PortID(*vlanPort.logicalPort_ref()) == port;
        });
    config.vlanPorts_ref()->erase(removed, config.vlanPorts_ref()->end());
  } else {
    cfgPort->state_ref() = cfg::PortState::DISABLED;
  }
}

void removeSubsumedPorts(
    cfg::SwitchConfig& config,
    const cfg::PlatformPortConfig& profile,
    bool supportsAddRemovePort) {
  if (auto subsumedPorts = profile.subsumedPorts_ref()) {
    for (auto& subsumedPortID : subsumedPorts.value()) {
      removePort(config, PortID(subsumedPortID), supportsAddRemovePort);
    }
  }
}

bool isRswPlatform(PlatformMode mode) {
  std::set rswPlatforms = {PlatformMode::WEDGE,
                           PlatformMode::WEDGE100,
                           PlatformMode::WEDGE400,
                           PlatformMode::WEDGE400C};
  return rswPlatforms.find(mode) == rswPlatforms.end();
}

} // unnamed namespace

namespace facebook::fboss::utility {

folly::MacAddress kLocalCpuMac() {
  static const folly::MacAddress kLocalMac("02:00:00:00:00:01");
  return kLocalMac;
}

cfg::SwitchConfig oneL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    cfg::PortLoopbackMode lbMode) {
  std::vector<PortID> ports{port};
  return oneL3IntfNPortConfig(hwSwitch, ports, lbMode);
}

cfg::SwitchConfig oneL3IntfNoIPAddrConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    cfg::PortLoopbackMode lbMode) {
  std::vector<PortID> ports{port};
  return oneL3IntfNPortConfig(
      hwSwitch, ports, lbMode, false /*interfaceHasSubnet*/);
}

cfg::SwitchConfig oneL3IntfTwoPortConfig(
    const HwSwitch* hwSwitch,
    PortID port1,
    PortID port2,
    cfg::PortLoopbackMode lbMode) {
  std::vector<PortID> ports{port1, port2};
  return oneL3IntfNPortConfig(hwSwitch, ports, lbMode);
}

cfg::SwitchConfig oneL3IntfNPortConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    cfg::PortLoopbackMode lbMode,
    bool interfaceHasSubnet) {
  std::map<PortID, VlanID> port2vlan;
  std::vector<VlanID> vlans{VlanID(kBaseVlanId)};
  std::vector<PortID> vlanPorts;
  for (auto port : ports) {
    port2vlan[port] = VlanID(kBaseVlanId);
    vlanPorts.push_back(port);
  }
  auto config = genPortVlanCfg(hwSwitch, vlanPorts, port2vlan, vlans, lbMode);

  config.interfaces_ref()->resize(1);
  *config.interfaces_ref()[0].intfID_ref() = kBaseVlanId;
  *config.interfaces_ref()[0].vlanID_ref() = kBaseVlanId;
  *config.interfaces_ref()[0].routerID_ref() = 0;
  config.interfaces_ref()[0].mac_ref() = getLocalCpuMacStr();
  config.interfaces_ref()[0].mtu_ref() = 9000;
  if (interfaceHasSubnet) {
    config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
    config.interfaces_ref()[0].ipAddresses_ref()[0] = "1.1.1.1/24";
    config.interfaces_ref()[0].ipAddresses_ref()[1] = "1::/64";
  }
  return config;
}

cfg::SwitchConfig onePortPerVlanConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    cfg::PortLoopbackMode lbMode,
    bool interfaceHasSubnet) {
  std::map<PortID, VlanID> port2vlan;
  std::vector<VlanID> vlans;
  std::vector<PortID> vlanPorts;
  auto idx = 0;
  for (auto port : ports) {
    auto vlan = kBaseVlanId + idx++;
    port2vlan[port] = VlanID(vlan);
    vlans.push_back(VlanID(vlan));
    vlanPorts.push_back(port);
  }
  auto config = genPortVlanCfg(hwSwitch, vlanPorts, port2vlan, vlans, lbMode);
  config.interfaces_ref()->resize(vlans.size());
  for (auto i = 0; i < vlans.size(); ++i) {
    *config.interfaces_ref()[i].intfID_ref() = kBaseVlanId + i;
    *config.interfaces_ref()[i].vlanID_ref() = kBaseVlanId + i;
    *config.interfaces_ref()[i].routerID_ref() = 0;
    config.interfaces_ref()[i].mac_ref() = getLocalCpuMacStr();
    config.interfaces_ref()[i].mtu_ref() = 9000;
    if (interfaceHasSubnet) {
      config.interfaces_ref()[i].ipAddresses_ref()->resize(2);
      auto ipDecimal = folly::sformat("{}", i + 1);
      config.interfaces_ref()[i].ipAddresses_ref()[0] =
          folly::sformat("{}.0.0.0/24", ipDecimal);
      config.interfaces_ref()[i].ipAddresses_ref()[1] =
          folly::sformat("{}::/64", ipDecimal);
    }
  }
  return config;
}

cfg::SwitchConfig twoL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port1,
    PortID port2,
    cfg::PortLoopbackMode lbMode) {
  std::map<PortID, VlanID> port2vlan;
  std::vector<PortID> ports;
  port2vlan[port1] = VlanID(kBaseVlanId);
  port2vlan[port2] = VlanID(kBaseVlanId);
  ports.push_back(port1);
  ports.push_back(port2);
  std::vector<VlanID> vlans = {VlanID(kBaseVlanId), VlanID(kBaseVlanId + 1)};
  auto config = genPortVlanCfg(hwSwitch, ports, port2vlan, vlans, lbMode);

  config.interfaces_ref()->resize(2);
  *config.interfaces_ref()[0].intfID_ref() = kBaseVlanId;
  *config.interfaces_ref()[0].vlanID_ref() = kBaseVlanId;
  *config.interfaces_ref()[0].routerID_ref() = 0;
  config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "1.1.1.1/24";
  config.interfaces_ref()[0].ipAddresses_ref()[1] = "1::1/64";
  *config.interfaces_ref()[1].intfID_ref() = kBaseVlanId + 1;
  *config.interfaces_ref()[1].vlanID_ref() = kBaseVlanId + 1;
  *config.interfaces_ref()[1].routerID_ref() = 0;
  config.interfaces_ref()[1].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[1].ipAddresses_ref()[0] = "2.2.2.2/24";
  config.interfaces_ref()[1].ipAddresses_ref()[1] = "2::1/64";
  for (auto& interface : *config.interfaces_ref()) {
    interface.mac_ref() = getLocalCpuMacStr();
    interface.mtu_ref() = 9000;
  }
  return config;
}

void addMatcher(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    const cfg::MatchAction& matchAction) {
  cfg::MatchToAction action = cfg::MatchToAction();
  *action.matcher_ref() = matcherName;
  *action.action_ref() = matchAction;
  cfg::TrafficPolicyConfig egressTrafficPolicy;
  if (auto dataPlaneTrafficPolicy = config->dataPlaneTrafficPolicy_ref()) {
    egressTrafficPolicy = *dataPlaneTrafficPolicy;
  }
  auto curNumMatchActions = egressTrafficPolicy.matchToAction_ref()->size();
  egressTrafficPolicy.matchToAction_ref()->resize(curNumMatchActions + 1);
  egressTrafficPolicy.matchToAction_ref()[curNumMatchActions] = action;
  config->dataPlaneTrafficPolicy_ref() = egressTrafficPolicy;
}

void updatePortProfile(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& cfg,
    PortID portID,
    cfg::PortProfileID profileID) {
  auto cfgPort = findCfgPort(cfg, portID);
  auto platform = hwSwitch.getPlatform();
  auto supportsAddRemovePort = platform->supportsAddRemovePort();
  auto platformPort = platform->getPlatformPort(portID);
  if (auto platPortEntry = platformPort->getPlatformPortEntry()) {
    const auto& supportedProfiles = *platPortEntry->supportedProfiles_ref();
    auto profile = supportedProfiles.find(profileID);
    if (profile != supportedProfiles.end()) {
      cfgPort->profileID_ref() = profileID;
      removeSubsumedPorts(cfg, profile->second, supportsAddRemovePort);
      cfgPort->speed_ref() =
          *platform->getPortProfileConfig(profileID)->speed_ref();
    }
  }
}

void updatePortSpeed(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& cfg,
    PortID portID,
    cfg::PortSpeed speed) {
  auto cfgPort = findCfgPort(cfg, portID);
  auto platform = hwSwitch.getPlatform();
  auto supportsAddRemovePort = platform->supportsAddRemovePort();
  auto platformPort = platform->getPlatformPort(portID);
  if (auto platPortEntry = platformPort->getPlatformPortEntry()) {
    auto profileID = platformPort->getProfileIDBySpeed(speed);
    const auto& supportedProfiles = *platPortEntry->supportedProfiles_ref();
    auto profile = supportedProfiles.find(profileID);
    if (profile == supportedProfiles.end()) {
      throw FbossError("No profile ", profileID, " found for port ", portID);
    }
    cfgPort->profileID_ref() = profileID;
    removeSubsumedPorts(cfg, profile->second, supportsAddRemovePort);
  }
  cfgPort->speed_ref() = speed;
}

std::vector<cfg::Port>::iterator findCfgPort(
    cfg::SwitchConfig& cfg,
    PortID portID) {
  auto port = findCfgPortIf(cfg, portID);
  if (port == cfg.ports_ref()->end()) {
    throw FbossError("No cfg found for port ", portID);
  }
  return port;
}

std::vector<cfg::Port>::iterator findCfgPortIf(
    cfg::SwitchConfig& cfg,
    PortID portID) {
  return std::find_if(
      cfg.ports_ref()->begin(), cfg.ports_ref()->end(), [&portID](auto& port) {
        return PortID(*port.logicalID_ref()) == portID;
      });
}

// Set any ports in this port group to use the specified speed,
// and disables any ports that don't support this speed.
void configurePortGroup(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& config,
    cfg::PortSpeed speed,
    std::vector<PortID> allPortsInGroup) {
  auto platform = hwSwitch.getPlatform();
  auto supportsAddRemovePort = platform->supportsAddRemovePort();
  for (auto portID : allPortsInGroup) {
    // We might have removed a subsumed port already in a previous
    // iteration of the loop.
    auto cfgPort = findCfgPortIf(config, portID);
    if (cfgPort == config.ports_ref()->end()) {
      continue;
    }

    auto platformPort = platform->getPlatformPort(portID);
    auto platPortEntry = platformPort->getPlatformPortEntry();
    if (platPortEntry == std::nullopt) {
      throw std::runtime_error(folly::to<std::string>(
          "No platform port entry found for port ", portID));
    }
    auto profileID = platformPort->getProfileIDBySpeedIf(speed);
    if (!profileID.has_value()) {
      XLOG(WARNING) << "Port " << static_cast<int>(portID)
                    << "Doesn't support speed " << static_cast<int>(speed)
                    << ", disabling it instead";
      // Port doesn't support this speed, just disable it.
      cfgPort->speed_ref() = cfg::PortSpeed::DEFAULT;
      cfgPort->state_ref() = cfg::PortState::DISABLED;
      continue;
    }

    auto supportedProfiles = *platPortEntry->supportedProfiles_ref();
    auto profile = supportedProfiles.find(profileID.value());
    if (profile == supportedProfiles.end()) {
      throw std::runtime_error(folly::to<std::string>(
          "No profile ", profileID.value(), " found for port ", portID));
    }

    cfgPort->profileID_ref() = profileID.value();
    cfgPort->speed_ref() = speed;
    cfgPort->state_ref() = cfg::PortState::ENABLED;
    removeSubsumedPorts(config, profile->second, supportsAddRemovePort);
  }
}

void configurePortProfile(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& config,
    cfg::PortProfileID profileID,
    std::vector<PortID> allPortsInGroup) {
  auto platform = hwSwitch.getPlatform();
  auto supportsAddRemovePort = platform->supportsAddRemovePort();
  auto speed = getPortSpeedFromProfile(platform, profileID);
  for (auto portID : allPortsInGroup) {
    // We might have removed a subsumed port already in a previous
    // iteration of the loop.
    auto cfgPort = findCfgPortIf(config, portID);
    if (cfgPort == config.ports_ref()->end()) {
      return;
    }

    auto platformPort = platform->getPlatformPort(portID);
    auto platPortEntry = platformPort->getPlatformPortEntry();
    if (platPortEntry == std::nullopt) {
      throw std::runtime_error(folly::to<std::string>(
          "No platform port entry found for port ", portID));
    }
    auto supportedProfiles = *platPortEntry->supportedProfiles_ref();
    auto profile = supportedProfiles.find(profileID);
    if (profile == supportedProfiles.end()) {
      XLOG(WARNING) << "Port " << static_cast<int>(portID)
                    << "Doesn't support profile " << static_cast<int>(profileID)
                    << ", disabling it instead";
      // Port doesn't support this speed, just disable it.
      cfgPort->speed_ref() = cfg::PortSpeed::DEFAULT;
      cfgPort->state_ref() = cfg::PortState::DISABLED;
      continue;
    }

    cfgPort->profileID_ref() = profileID;
    cfgPort->speed_ref() = speed;
    cfgPort->state_ref() = cfg::PortState::ENABLED;
    removeSubsumedPorts(config, profile->second, supportsAddRemovePort);
  }
}

std::string getAsicChipFromPortID(const HwSwitch* hwSwitch, PortID id) {
  auto platform = hwSwitch->getPlatform();
  std::string chip;
  if (auto entry = platform->getPlatformPort(id)->getPlatformPortEntry()) {
    auto pins = entry->mapping_ref()->pins_ref();
    if (!pins->empty()) {
      chip = *(pins->front().a_ref()->chip_ref());
    } else {
      XLOG(DBG5) << "No pins for port " << id << ", return empty chip name";
    }
  } else {
    XLOG(DBG5) << "No platformPortEntry for port " << id
               << ", return empty chip name";
  }
  return chip;
}

std::vector<PortID> getAllPortsInGroup(
    const HwSwitch* hwSwitch,
    PortID portID) {
  std::vector<PortID> allPortsinGroup;
  if (const auto& platformPorts = hwSwitch->getPlatform()->getPlatformPorts();
      !platformPorts.empty()) {
    const auto& portList =
        utility::getPlatformPortsByControllingPort(platformPorts, portID);
    for (const auto& port : portList) {
      allPortsinGroup.push_back(PortID(*port.mapping_ref()->id_ref()));
    }
  }
  return allPortsinGroup;
}

cfg::SwitchConfig createUplinkDownlinkConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& masterLogicalPortIds,
    uint16_t uplinksCount,
    cfg::PortSpeed uplinkPortSpeed,
    cfg::PortSpeed downlinkPortSpeed,
    cfg::PortLoopbackMode lbMode,
    bool interfaceHasSubnet) {
  auto platform = hwSwitch->getPlatform();
  /*
   * For platforms which are not rsw, its always onePortPerVlanConfig
   * config with all uplinks and downlinks in same speed. Use the
   * config factory utility to generate the config, update the port
   * speed and return the config.
   */
  if (isRswPlatform(platform->getMode())) {
    auto config =
        utility::onePortPerVlanConfig(hwSwitch, masterLogicalPortIds, lbMode);
    for (auto portId : masterLogicalPortIds) {
      utility::updatePortSpeed(*hwSwitch, config, portId, uplinkPortSpeed);
    }
    return config;
  }

  /*
   * Configure the top ports in the master logical port ids as uplinks
   * and remaining as downlinks based on the platform
   */
  std::vector<PortID> uplinkMasterPorts(
      masterLogicalPortIds.begin(),
      masterLogicalPortIds.begin() + uplinksCount);
  std::vector<PortID> downlinkMasterPorts(
      masterLogicalPortIds.begin() + uplinksCount, masterLogicalPortIds.end());
  /*
   * Prod uplinks are always onePortPerVlanConfig. Use the existing
   * utlity to generate one port per vlan config followed by port
   * speed update.
   */
  auto config = utility::onePortPerVlanConfig(
      hwSwitch, uplinkMasterPorts, lbMode, interfaceHasSubnet);
  for (auto portId : uplinkMasterPorts) {
    utility::updatePortSpeed(*hwSwitch, config, portId, uplinkPortSpeed);
  }

  /*
   * downlinkMasterPorts are master logical port ids. Get all the ports in
   * a port group and add them to the config. Use configurePortGroup
   * to set the right speed and remove/disable the subsumed ports
   * based on the platform.
   */
  std::vector<PortID> allDownlinkPorts;
  for (auto masterDownlinkPort : downlinkMasterPorts) {
    auto allDownlinkPortsInGroup =
        utility::getAllPortsInGroup(hwSwitch, masterDownlinkPort);
    for (auto logicalPortId : allDownlinkPortsInGroup) {
      auto portConfig = findCfgPortIf(config, masterDownlinkPort);
      if (portConfig != config.ports_ref()->end()) {
        allDownlinkPorts.push_back(logicalPortId);
      }
    }
    configurePortGroup(
        *hwSwitch, config, downlinkPortSpeed, allDownlinkPortsInGroup);
  }

  // Vlan config
  cfg::Vlan vlan;
  vlan.id_ref() = kDownlinkBaseVlanId;
  vlan.name_ref() = "vlan" + std::to_string(kDownlinkBaseVlanId);
  vlan.routable_ref() = true;
  config.vlans_ref()->push_back(vlan);

  // Vlan port config
  for (auto logicalPortId : allDownlinkPorts) {
    auto portConfig = utility::findCfgPortIf(config, logicalPortId);
    if (portConfig == config.ports_ref()->end()) {
      continue;
    }
    portConfig->loopbackMode_ref() = lbMode;
    portConfig->ingressVlan_ref() = kDownlinkBaseVlanId;
    portConfig->routable_ref() = true;
    portConfig->parserType_ref() = cfg::ParserType::L3;
    portConfig->maxFrameSize_ref() = 9412;
    cfg::VlanPort vlanPort;
    vlanPort.logicalPort_ref() = logicalPortId;
    vlanPort.vlanID_ref() = kDownlinkBaseVlanId;
    vlanPort.spanningTreeState_ref() = cfg::SpanningTreeState::FORWARDING;
    vlanPort.emitTags_ref() = false;
    config.vlanPorts_ref()->push_back(vlanPort);
  }

  cfg::Interface interface;
  interface.intfID_ref() = kDownlinkBaseVlanId;
  interface.vlanID_ref() = kDownlinkBaseVlanId;
  interface.routerID_ref() = 0;
  interface.mac_ref() = utility::kLocalCpuMac().toString();
  interface.mtu_ref() = 9000;
  config.interfaces_ref()->push_back(interface);

  return config;
}
} // namespace facebook::fboss::utility
