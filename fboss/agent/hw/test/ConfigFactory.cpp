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
  if (auto maxSpeed = hwSwitch->getPortMaxSpeed(port);
      maxSpeed != cfg::PortSpeed::DEFAULT) {
    return maxSpeed;
  }
  return hwSwitch->getPlatform()->getPortMaxSpeed(port);
}

cfg::SwitchConfig genPortVlanCfg(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    const std::map<PortID, VlanID>& port2vlan,
    const std::vector<VlanID>& vlans,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE) {
  cfg::SwitchConfig config;

  // Port config
  config.ports_ref()->resize(ports.size());
  auto portItr = ports.begin();
  int portIndex = 0;
  for (; portItr != ports.end(); portItr++, portIndex++) {
    *config.ports[portIndex].logicalID_ref() = *portItr;
    *config.ports[portIndex].speed_ref() = maxPortSpeed(hwSwitch, *portItr);
    auto platformPort = hwSwitch->getPlatform()->getPlatformPort(*portItr);
    if (auto entry = platformPort->getPlatformPortEntry()) {
      config.ports_ref()[portIndex].name_ref() =
          *entry->mapping_ref()->name_ref();
      *config.ports[portIndex].profileID_ref() =
          platformPort->getProfileIDBySpeed(
              *config.ports[portIndex].speed_ref());
      if (*config.ports[portIndex].profileID_ref() ==
          cfg::PortProfileID::PROFILE_DEFAULT) {
        throw FbossError(
            *entry->mapping_ref()->name_ref(),
            " has speed: ",
            apache::thrift::util::enumNameSafe(
                *config.ports[portIndex].speed_ref()),
            " which has profile: ",
            apache::thrift::util::enumNameSafe(
                *config.ports[portIndex].profileID_ref()));
      }
    } else {
      config.ports_ref()[portIndex].name_ref() =
          "eth1/" + std::to_string(*portItr) + "/1";
    }
    *config.ports[portIndex].maxFrameSize_ref() = 9412;
    *config.ports[portIndex].state_ref() = cfg::PortState::ENABLED;
    *config.ports[portIndex].loopbackMode_ref() = lbMode;
    *config.ports[portIndex].ingressVlan_ref() =
        port2vlan.find(*portItr)->second;
    *config.ports[portIndex].routable_ref() = true;
    *config.ports[portIndex].parserType_ref() = cfg::ParserType::L3;
  }

  // Vlan config
  config.vlans_ref()->resize(vlans.size() + 1);
  auto vlanCfgItr = vlans.begin();
  int vlanIndex = 0;
  for (; vlanCfgItr != vlans.end(); vlanCfgItr++, vlanIndex++) {
    *config.vlans[vlanIndex].id_ref() = *vlanCfgItr;
    *config.vlans[vlanIndex].name_ref() = "vlan" + std::to_string(*vlanCfgItr);
    *config.vlans[vlanIndex].routable_ref() = true;
  }
  *config.vlans[vlanIndex].id_ref() = kDefaultVlanId;
  *config.vlans[vlanIndex].name_ref() =
      folly::sformat("vlan{}", kDefaultVlanId);
  *config.vlans[vlanIndex].routable_ref() = true;

  *config.defaultVlan_ref() = kDefaultVlanId;

  // Vlan port config
  config.vlanPorts_ref()->resize(port2vlan.size());
  auto vlanItr = port2vlan.begin();
  portIndex = 0;
  for (; vlanItr != port2vlan.end(); vlanItr++, portIndex++) {
    *config.vlanPorts[portIndex].logicalPort_ref() = vlanItr->first;
    *config.vlanPorts[portIndex].vlanID_ref() = vlanItr->second;
    *config.vlanPorts[portIndex].spanningTreeState_ref() =
        cfg::SpanningTreeState::FORWARDING;
    *config.vlanPorts[portIndex].emitTags_ref() = false;
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
  if (cfgPort == config.ports.end()) {
    return;
  }
  if (supportsAddRemovePort) {
    config.ports.erase(cfgPort);
    auto removed = std::remove_if(
        config.vlanPorts.begin(),
        config.vlanPorts.end(),
        [port](auto vlanPort) { return PortID(vlanPort.logicalPort) == port; });
    config.vlanPorts.erase(removed, config.vlanPorts.end());
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
  *config.interfaces[0].intfID_ref() = kBaseVlanId;
  *config.interfaces[0].vlanID_ref() = kBaseVlanId;
  *config.interfaces[0].routerID_ref() = 0;
  config.interfaces_ref()[0].mac_ref() = getLocalCpuMacStr();
  config.interfaces_ref()[0].mtu_ref() = 9000;
  if (interfaceHasSubnet) {
    config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
    config.interfaces[0].ipAddresses_ref()[0] = "1.1.1.1/24";
    config.interfaces[0].ipAddresses_ref()[1] = "1::/64";
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
    *config.interfaces[i].intfID_ref() = kBaseVlanId + i;
    *config.interfaces[i].vlanID_ref() = kBaseVlanId + i;
    *config.interfaces[i].routerID_ref() = 0;
    config.interfaces_ref()[i].mac_ref() = getLocalCpuMacStr();
    config.interfaces_ref()[i].mtu_ref() = 9000;
    if (interfaceHasSubnet) {
      config.interfaces_ref()[i].ipAddresses_ref()->resize(2);
      auto ipDecimal = folly::sformat("{}", i + 1);
      config.interfaces[i].ipAddresses_ref()[0] =
          folly::sformat("{}.0.0.0/24", ipDecimal);
      config.interfaces[i].ipAddresses_ref()[1] =
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
  *config.interfaces[0].intfID_ref() = kBaseVlanId;
  *config.interfaces[0].vlanID_ref() = kBaseVlanId;
  *config.interfaces[0].routerID_ref() = 0;
  config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
  config.interfaces[0].ipAddresses_ref()[0] = "1.1.1.1/24";
  config.interfaces[0].ipAddresses_ref()[1] = "1::1/64";
  *config.interfaces[1].intfID_ref() = kBaseVlanId + 1;
  *config.interfaces[1].vlanID_ref() = kBaseVlanId + 1;
  *config.interfaces[1].routerID_ref() = 0;
  config.interfaces_ref()[1].ipAddresses_ref()->resize(2);
  config.interfaces[1].ipAddresses_ref()[0] = "2.2.2.2/24";
  config.interfaces[1].ipAddresses_ref()[1] = "2::1/64";
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
  if (auto platPortEntry = platformPort->getPlatformPortEntry()) {
    auto profileID = platformPort->getProfileIDBySpeed(speed);
    const auto& supportedProfiles = platPortEntry->supportedProfiles;
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
  if (port == cfg.ports.end()) {
    throw FbossError("No cfg found for port ", portID);
  }
  return port;
}

std::vector<cfg::Port>::iterator findCfgPortIf(
    cfg::SwitchConfig& cfg,
    PortID portID) {
  return std::find_if(
      cfg.ports.begin(), cfg.ports.end(), [&portID](auto& port) {
        return PortID(port.logicalID) == portID;
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
    if (cfgPort == config.ports.end()) {
      return;
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

    auto supportedProfiles = platPortEntry->supportedProfiles;
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

} // namespace facebook::fboss::utility
