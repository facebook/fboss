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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwPortUtils.h"
#include "fboss/agent/platforms/common/PlatformMode.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

DEFINE_bool(nodeZ, false, "Setup test config as node Z");

namespace {

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
  std::set rswPlatforms = {
      PlatformMode::WEDGE,
      PlatformMode::WEDGE100,
      PlatformMode::WEDGE400,
      PlatformMode::WEDGE400C};
  return rswPlatforms.find(mode) != rswPlatforms.end();
}
} // unnamed namespace

namespace facebook::fboss::utility {

std::unordered_map<PortID, cfg::PortProfileID>& getPortToDefaultProfileIDMap() {
  static std::unordered_map<PortID, cfg::PortProfileID> portProfileIDMap;
  return portProfileIDMap;
}

namespace {

cfg::Port createDefaultPortConfig(
    const Platform* platform,
    PortID id,
    cfg::PortProfileID defaultProfileID) {
  cfg::Port defaultConfig;
  const auto& entry = platform->getPlatformPort(id)->getPlatformPortEntry();
  defaultConfig.name_ref() = *entry.mapping_ref()->name_ref();
  defaultConfig.speed_ref() = getSpeed(defaultProfileID);
  defaultConfig.profileID_ref() = defaultProfileID;

  defaultConfig.logicalID_ref() = id;
  defaultConfig.ingressVlan_ref() = kDefaultVlanId;
  defaultConfig.state_ref() = cfg::PortState::DISABLED;
  return defaultConfig;
}

std::unordered_map<PortID, cfg::PortProfileID> getSafeProfileIDs(
    const Platform* platform,
    const std::map<PortID, std::vector<PortID>>&
        controllingPortToSubsidaryPorts) {
  std::unordered_map<PortID, cfg::PortProfileID> portToProfileIDs;
  const auto& plarformEntries = platform->getPlatformPorts();
  for (const auto& group : controllingPortToSubsidaryPorts) {
    const auto& ports = group.second;
    // Find the safe profile to satisfy all the ports in the group
    std::set<cfg::PortProfileID> safeProfiles;
    for (const auto portID : ports) {
      for (const auto& profile :
           *plarformEntries.at(portID).supportedProfiles_ref()) {
        if (auto subsumedPorts = profile.second.subsumedPorts_ref();
            subsumedPorts && !subsumedPorts->empty()) {
          // as long as subsumedPorts doesn't overlap with portSet, also safe
          if (std::none_of(
                  subsumedPorts->begin(),
                  subsumedPorts->end(),
                  [ports](auto subsumedPort) {
                    return std::find(
                               ports.begin(),
                               ports.end(),
                               PortID(subsumedPort)) != ports.end();
                  })) {
            safeProfiles.insert(profile.first);
          }
        } else {
          // no subsumed ports for this profile, safe
          safeProfiles.insert(profile.first);
        }
      }
    }
    if (safeProfiles.empty()) {
      std::string portSetStr = "";
      for (auto portID : ports) {
        portSetStr = folly::to<std::string>(portSetStr, portID, ", ");
      }
      throw FbossError("Can't find safe profiles for ports:", portSetStr);
    }
    // Always pick the largest speed from the safe profiles
    auto bestSpeed = cfg::PortSpeed::DEFAULT;
    auto bestProfile = cfg::PortProfileID::PROFILE_DEFAULT;
    for (auto profileID : safeProfiles) {
      auto speed = getSpeed(profileID);
      if (static_cast<int>(bestSpeed) < static_cast<int>(speed)) {
        bestSpeed = speed;
        bestProfile = profileID;
      }
    }

    for (auto portID : ports) {
      portToProfileIDs.emplace(portID, bestProfile);
    }
  }
  return portToProfileIDs;
}

void securePortsInConfig(
    const Platform* platform,
    cfg::SwitchConfig& config,
    const std::vector<PortID>& ports) {
  // This function is to secure all ports in the input `ports` vector will be
  // in the config. Usually there're two main cases:
  // 1) all the ports in ports vector are from different group, so we don't need
  // to worry about how to deal w/ the slave ports.
  // 2) some ports in the ports vector are in the same group. In this case, we
  // need to make sure these ports will be in the config, and what's the most
  // important, we also need to make sure these ports using a safe PortProfileID
  std::map<PortID, std::vector<PortID>> groupPortsByControllingPort;
  const auto& plarformEntries = platform->getPlatformPorts();
  for (const auto portID : ports) {
    if (const auto& entry = plarformEntries.find(portID);
        entry != plarformEntries.end()) {
      auto controllingPort =
          PortID(*entry->second.mapping_ref()->controllingPort_ref());
      groupPortsByControllingPort[controllingPort].push_back(portID);
    } else {
      throw FbossError("Port:", portID, " doesn't exist in PlatformMapping");
    }
  }

  // If the mandatory port from input `ports` is the only port from the same
  // port group, and it already exists in the config, we don't need to adjust
  // the profileID and speed for it.
  std::map<PortID, std::vector<PortID>> portGroups;
  for (auto group : groupPortsByControllingPort) {
    auto portSet = group.second;
    if (portSet.size() == 1 &&
        findCfgPortIf(config, *portSet.begin()) != config.ports_ref()->end()) {
      continue;
    }
    portGroups.emplace(group.first, std::move(group.second));
  }

  // Make sure all the ports in portGroups use the safe profile in the config
  if (portGroups.size() > 0) {
    for (const auto& [portID, profileID] :
         getSafeProfileIDs(platform, portGroups)) {
      auto portCfg = findCfgPortIf(config, portID);
      if (portCfg != config.ports_ref()->end()) {
        portCfg->profileID_ref() = profileID;
        portCfg->speed_ref() = getSpeed(profileID);
      } else {
        config.ports_ref()->push_back(
            createDefaultPortConfig(platform, portID, profileID));
      }
    }
  }
}

cfg::SwitchConfig genPortVlanCfg(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    const std::map<PortID, VlanID>& port2vlan,
    const std::vector<VlanID>& vlans,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE,
    bool optimizePortProfile = true) {
  cfg::SwitchConfig config;
  // Use getPortToDefaultProfileIDMap() to genetate the default config instead
  // of using PlatformMapping.
  // The main reason is to avoid using PlatformMapping is because some of the
  // platforms support adding and removing ports, and their ChipConfig might
  // only has the controlling ports, which means when the chip is initialized
  // the number of ports from the hardware might be different from the number
  // of total platform ports from PlatformMapping.
  // And if we try to use all ports from PlatformMapping to create default
  // config, it will have to trigger an unncessary PortGroup re-program to add
  // those new ports on the hardware.
  const auto& portToDefaultProfileID = getPortToDefaultProfileIDMap();
  CHECK_GT(portToDefaultProfileID.size(), 0);
  for (auto const& [portID, profileID] : portToDefaultProfileID) {
    config.ports_ref()->push_back(
        createDefaultPortConfig(hwSwitch->getPlatform(), portID, profileID));
  }

  // Secure all ports in `ports` vector in the config
  securePortsInConfig(hwSwitch->getPlatform(), config, ports);

  // Port config
  for (auto portID : ports) {
    auto portCfg = findCfgPort(config, portID);
    portCfg->maxFrameSize_ref() = 9412;
    portCfg->state_ref() = cfg::PortState::ENABLED;
    portCfg->loopbackMode_ref() = lbMode;
    portCfg->ingressVlan_ref() = port2vlan.find(portID)->second;
    portCfg->routable_ref() = true;
    portCfg->parserType_ref() = cfg::ParserType::L3;
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
} // namespace

void setPortToDefaultProfileIDMap(
    const std::shared_ptr<PortMap>& ports,
    const Platform* platform) {
  // Most of the platforms will have default ports created when the HW is
  // initialized. But for those who don't have any default port, we'll fall
  // back to use PlatformPort and the safe PortProfileID
  if (ports->numPorts() > 0) {
    for (const auto& port : *ports) {
      auto profileID = port->getProfileID();
      // In case the profileID learnt from HW is using default, then use speed
      // to get the real profileID
      if (profileID == cfg::PortProfileID::PROFILE_DEFAULT) {
        auto platformPort = platform->getPlatformPort(port->getID());
        profileID = platformPort->getProfileIDBySpeed(port->getSpeed());
      }
      getPortToDefaultProfileIDMap().emplace(port->getID(), profileID);
    }
  } else {
    const auto& safeProfileIDs = getSafeProfileIDs(
        platform, getSubsidiaryPortIDs(platform->getPlatformPorts()));
    getPortToDefaultProfileIDMap().insert(
        safeProfileIDs.begin(), safeProfileIDs.end());
  }
  XLOG(INFO) << "PortToDefaultProfileIDMap has "
             << getPortToDefaultProfileIDMap().size() << " ports";
}

folly::MacAddress kLocalCpuMac() {
  static const folly::MacAddress kLocalMac(
      FLAGS_nodeZ ? "02:00:00:00:00:02" : "02:00:00:00:00:01");
  return kLocalMac;
}

cfg::SwitchConfig oneL3IntfConfig(
    const HwSwitch* hwSwitch,
    PortID port,
    cfg::PortLoopbackMode lbMode,
    int baseVlanId) {
  std::vector<PortID> ports{port};
  return oneL3IntfNPortConfig(hwSwitch, ports, lbMode, true, baseVlanId);
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
    bool interfaceHasSubnet,
    int baseVlanId,
    bool optimizePortProfile,
    bool setInterfaceMac) {
  std::map<PortID, VlanID> port2vlan;
  std::vector<VlanID> vlans{VlanID(baseVlanId)};
  std::vector<PortID> vlanPorts;
  for (auto port : ports) {
    port2vlan[port] = VlanID(baseVlanId);
    vlanPorts.push_back(port);
  }
  auto config = genPortVlanCfg(
      hwSwitch, vlanPorts, port2vlan, vlans, lbMode, optimizePortProfile);

  config.interfaces_ref()->resize(1);
  config.interfaces_ref()[0].intfID_ref() = baseVlanId;
  config.interfaces_ref()[0].vlanID_ref() = baseVlanId;
  *config.interfaces_ref()[0].routerID_ref() = 0;
  if (setInterfaceMac) {
    config.interfaces_ref()[0].mac_ref() = getLocalCpuMacStr();
  }
  config.interfaces_ref()[0].mtu_ref() = 9000;
  if (interfaceHasSubnet) {
    config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
    config.interfaces_ref()[0].ipAddresses_ref()[0] =
        FLAGS_nodeZ ? "1.1.1.2/24" : "1.1.1.1/24";
    config.interfaces_ref()[0].ipAddresses_ref()[1] =
        FLAGS_nodeZ ? "1::1/64" : "1::/64";
  }
  return config;
}

cfg::SwitchConfig onePortPerVlanConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    cfg::PortLoopbackMode lbMode,
    bool interfaceHasSubnet,
    bool setInterfaceMac) {
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
    if (setInterfaceMac) {
      config.interfaces_ref()[i].mac_ref() = getLocalCpuMacStr();
    }
    config.interfaces_ref()[i].mtu_ref() = 9000;
    if (interfaceHasSubnet) {
      config.interfaces_ref()[i].ipAddresses_ref()->resize(2);
      auto ipDecimal = folly::sformat("{}", i + 1);
      config.interfaces_ref()[i].ipAddresses_ref()[0] = FLAGS_nodeZ
          ? folly::sformat("{}.0.0.1/24", ipDecimal)
          : folly::sformat("{}.0.0.0/24", ipDecimal);
      config.interfaces_ref()[i].ipAddresses_ref()[1] = FLAGS_nodeZ
          ? folly::sformat("{}::1/64", ipDecimal)
          : folly::sformat("{}::0/64", ipDecimal);
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

void updatePortSpeed(
    const HwSwitch& hwSwitch,
    cfg::SwitchConfig& cfg,
    PortID portID,
    cfg::PortSpeed speed) {
  auto cfgPort = findCfgPort(cfg, portID);
  auto platform = hwSwitch.getPlatform();
  auto supportsAddRemovePort = platform->supportsAddRemovePort();
  auto platformPort = platform->getPlatformPort(portID);
  const auto& platPortEntry = platformPort->getPlatformPortEntry();
  auto profileID = platformPort->getProfileIDBySpeed(speed);
  const auto& supportedProfiles = *platPortEntry.supportedProfiles_ref();
  auto profile = supportedProfiles.find(profileID);
  if (profile == supportedProfiles.end()) {
    throw FbossError("No profile ", profileID, " found for port ", portID);
  }
  cfgPort->profileID_ref() = profileID;
  removeSubsumedPorts(cfg, profile->second, supportsAddRemovePort);
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
    const auto& platPortEntry = platformPort->getPlatformPortEntry();
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

    auto supportedProfiles = *platPortEntry.supportedProfiles_ref();
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
    std::vector<PortID> allPortsInGroup,
    PortID controllingPortID) {
  auto platform = hwSwitch.getPlatform();
  auto supportsAddRemovePort = platform->supportsAddRemovePort();
  auto controllingPort = findCfgPort(config, controllingPortID);
  for (auto portID : allPortsInGroup) {
    // We might have removed a subsumed port already in a previous
    // iteration of the loop.
    auto cfgPort = findCfgPortIf(config, portID);
    if (cfgPort == config.ports_ref()->end()) {
      return;
    }

    auto platformPort = platform->getPlatformPort(portID);
    const auto& platPortEntry = platformPort->getPlatformPortEntry();
    auto supportedProfiles = *platPortEntry.supportedProfiles_ref();
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
    cfgPort->speed_ref() = getSpeed(profileID);
    cfgPort->ingressVlan_ref() = *controllingPort->ingressVlan_ref();
    cfgPort->state_ref() = cfg::PortState::ENABLED;
    removeSubsumedPorts(config, profile->second, supportsAddRemovePort);
  }
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
  if (!isRswPlatform(platform->getMode())) {
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
      hwSwitch,
      uplinkMasterPorts,
      lbMode,
      interfaceHasSubnet);
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
  if (interfaceHasSubnet) {
    interface.ipAddresses_ref()->resize(2);
    interface.ipAddresses_ref()[0] = "192.1.1.1/24";
    interface.ipAddresses_ref()[1] = "2192::1/64";
  }
  config.interfaces_ref()->push_back(interface);

  return config;
}

/*
 * Returns a pair of vectors of the form (uplinks, downlinks). Pertains
 * specifically to default RSW platforms, where all downlink ports are
 * configured to be on ingressVlan 2000.
 *
 * Created to verify load balancing based on the switch's config, but can be
 * used anywhere it's useful. Likely will also be used in verifying
 * queue-per-host QoS for MHNIC platforms.
 */
UplinkDownlinkPair getRswUplinkDownlinkPorts(const cfg::SwitchConfig& config) {
  std::vector<PortID> uplinks, downlinks;

  auto confPorts = *config.ports_ref();
  XLOG_IF(WARN, confPorts.empty()) << "no ports found in config.ports_ref()";

  for (const auto& port : confPorts) {
    auto logId = port.get_logicalID();
    if (port.get_state() != cfg::PortState::ENABLED) {
      continue;
    }

    auto portId = PortID(logId);
    auto vlanId = port.get_ingressVlan();
    if (vlanId == kDownlinkBaseVlanId) {
      downlinks.push_back(portId);
    } else {
      uplinks.push_back(portId);
    }
  }

  return std::pair(uplinks, downlinks);
}

/*
 * Generic function to separate uplinks and downlinks, given a HwSwitch* and a
 * SwitchConfig.
 *
 * Calls already-defined RSW helper functions if HwSwitch* is an RSW platform,
 * otherwise separates the config's ports.
 */
UplinkDownlinkPair getAllUplinkDownlinkPorts(
    const HwSwitch* hwSwitch,
    const cfg::SwitchConfig& config,
    int kEcmpWidth) {
  auto platMode = hwSwitch->getPlatform()->getMode();
  if (isRswPlatform(platMode)) {
    return getRswUplinkDownlinkPorts(config);
  }

  // If the platform is not an RSW, consider the first kEcmpWidth-many ports to
  // be uplinks and the rest to be downlinks.
  // First populate masterPorts with all ports, analogous to
  // masterLogicalPortIds, then slice uplinks/downlinks according to kEcmpWidth.

  // just for brevity, mostly in return statement
  using PortList = std::vector<PortID>;
  PortList masterPorts;

  for (const auto& port : *config.ports_ref()) {
    if (port.get_state() == cfg::PortState::ENABLED) {
      masterPorts.push_back(PortID(port.get_logicalID()));
    }
  }

  auto begin = masterPorts.begin();
  auto mid = masterPorts.begin() + kEcmpWidth;
  auto end = masterPorts.end();
  return std::pair(PortList(begin, mid), PortList(mid, end));
}

/*
 * Takes a SwitchConfig and returns a map of queue IDs to DSCPs.
 * Particularly useful in verifyQueueMappings, where we don't have a guarantee
 * of what the QoS policies look like and we can't rely on something like
 * kOlympicQueueToDscp().
 */
std::map<int, std::vector<uint8_t>> getOlympicQosMaps(
    const cfg::SwitchConfig& config) {
  std::map<int, std::vector<uint8_t>> queueToDscp;

  for (const auto& qosPolicy : *config.qosPolicies_ref()) {
    auto qosName = qosPolicy.get_name();
    XLOG(INFO) << "Iterating over QoS policies: found qosPolicy " << qosName;

    // Optional thrift field access
    if (auto qosMap = qosPolicy.qosMap_ref()) {
      auto dscpMaps = *qosMap->dscpMaps_ref();

      for (const auto& dscpMap : dscpMaps) {
        auto queueId = dscpMap.get_internalTrafficClass();
        // Internally (i.e. in thrift), the mapping is implemented as a
        // map<int16_t, vector<int8_t>>; however, in functions like
        // verifyQueueMapping in HwTestQosUtils, the argument used is of the
        // form map<int, uint8_t>.
        // Trying to assign vector<uint8_t> to a vector<int8_t> makes the STL
        // unhappy, so we can just loop through and construct one on our own.
        std::vector<uint8_t> dscps;
        for (auto val : *dscpMap.fromDscpToTrafficClass_ref()) {
          dscps.push_back((uint8_t)val);
        }
        queueToDscp[(int)queueId] = std::move(dscps);
      }
    } else {
      XLOG(ERR) << "qosMap not found in qosPolicy: " << qosName;
    }
  }

  return queueToDscp;
}

} // namespace facebook::fboss::utility
