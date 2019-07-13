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

#include <folly/Format.h>

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

namespace {

cfg::PortSpeed maxPortSpeed(const HwSwitch* hwSwitch, PortID port) {
  return hwSwitch->getPortMaxSpeed(port);
}

cfg::SwitchConfig genPortVlanCfg(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports,
    const std::map<PortID, VlanID>& port2vlan,
    const std::vector<VlanID>& vlans,
    cfg::PortLoopbackMode lbMode = cfg::PortLoopbackMode::NONE) {
  cfg::SwitchConfig config;

  // Port config
  config.ports.resize(ports.size());
  auto portItr = ports.begin();
  int portIndex = 0;
  for (; portItr != ports.end(); portItr++, portIndex++) {
    config.ports[portIndex].name_ref() =
        "eth1/" + std::to_string(*portItr) + "/1";
    config.ports[portIndex].logicalID = *portItr;
    config.ports[portIndex].speed = maxPortSpeed(hwSwitch, *portItr);
    config.ports[portIndex].state = cfg::PortState::ENABLED;
    config.ports[portIndex].loopbackMode = lbMode;
    config.ports[portIndex].ingressVlan = port2vlan.find(*portItr)->second;
    config.ports[portIndex].routable = true;
    config.ports[portIndex].parserType = cfg::ParserType::L3;
  }

  // Vlan config
  config.vlans.resize(vlans.size() + 1);
  auto vlanCfgItr = vlans.begin();
  int vlanIndex = 0;
  for (; vlanCfgItr != vlans.end(); vlanCfgItr++, vlanIndex++) {
    config.vlans[vlanIndex].id = *vlanCfgItr;
    config.vlans[vlanIndex].name = "vlan" + std::to_string(*vlanCfgItr);
    config.vlans[vlanIndex].routable = true;
  }
  config.vlans[vlanIndex].id = kDefaultVlanId;
  config.vlans[vlanIndex].name = folly::sformat("vlan{}", kDefaultVlanId);
  config.vlans[vlanIndex].routable = true;

  config.defaultVlan = kDefaultVlanId;

  // Vlan port config
  config.vlanPorts.resize(port2vlan.size());
  auto vlanItr = port2vlan.begin();
  portIndex = 0;
  for (; vlanItr != port2vlan.end(); vlanItr++, portIndex++) {
    config.vlanPorts[portIndex].logicalPort = vlanItr->first;
    config.vlanPorts[portIndex].vlanID = vlanItr->second;
    config.vlanPorts[portIndex].spanningTreeState =
        cfg::SpanningTreeState::FORWARDING;
    config.vlanPorts[portIndex].emitTags = false;
  }

  return config;
}

std::string getLocalCpuMacStr() {
  return kLocalCpuMac().toString();
}
} // unnamed namespace

namespace facebook {
namespace fboss {
namespace utility {

folly::MacAddress kLocalCpuMac() {
  static const folly::MacAddress kLocalMac("02:00:00:00:00:01");
  return kLocalMac;
}

cfg::SwitchConfig onePortConfig(const HwSwitch* hwSwitch, PortID port) {
  std::map<PortID, VlanID> port2vlan;
  std::vector<PortID> ports;
  port2vlan[port] = VlanID(kDefaultVlanId);
  ports.push_back(port);
  return genPortVlanCfg(hwSwitch, ports, port2vlan, std::vector<VlanID>({}));
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

  config.interfaces.resize(1);
  config.interfaces[0].intfID = kBaseVlanId;
  config.interfaces[0].vlanID = kBaseVlanId;
  config.interfaces[0].routerID = 0;
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].mac_ref().value_unchecked() = getLocalCpuMacStr();
  if (interfaceHasSubnet) {
    config.interfaces[0].ipAddresses.resize(2);
    config.interfaces[0].ipAddresses[0] = "1.1.1.1/24";
    config.interfaces[0].ipAddresses[1] = "1::/64";
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
  config.interfaces.resize(vlans.size());
  for (auto i = 0; i < vlans.size(); ++i) {
    config.interfaces[i].intfID = kBaseVlanId + i;
    config.interfaces[i].vlanID = kBaseVlanId + i;
    config.interfaces[i].routerID = 0;
    config.interfaces[i].__isset.mac = true;
    config.interfaces[i].mac_ref().value_unchecked() = getLocalCpuMacStr();
    if (interfaceHasSubnet) {
      config.interfaces[i].ipAddresses.resize(2);
      auto ipDecimal = folly::sformat("{}", i + 1);
      config.interfaces[i].ipAddresses[0] =
          folly::sformat("{}.0.0.0/24", ipDecimal);
      config.interfaces[i].ipAddresses[1] =
          folly::sformat("{}::/64", ipDecimal);
    }
  }
  return config;
}

cfg::SwitchConfig
twoL3IntfConfig(const HwSwitch* hwSwitch, PortID port1, PortID port2) {
  std::map<PortID, VlanID> port2vlan;
  std::vector<PortID> ports;
  port2vlan[port1] = VlanID(kBaseVlanId);
  port2vlan[port2] = VlanID(kBaseVlanId);
  ports.push_back(port1);
  ports.push_back(port2);
  std::vector<VlanID> vlans = {VlanID(kBaseVlanId), VlanID(kBaseVlanId + 1)};
  auto config = genPortVlanCfg(hwSwitch, ports, port2vlan, vlans);

  config.interfaces.resize(2);
  config.interfaces[0].intfID = kBaseVlanId;
  config.interfaces[0].vlanID = kBaseVlanId;
  config.interfaces[0].routerID = 0;
  // Locally adminstered MAC
  config.interfaces[0].mac_ref().value_unchecked() = getLocalCpuMacStr();
  config.interfaces[0].__isset.mac = true;
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "1.1.1.1/24";
  config.interfaces[0].ipAddresses[1] = "1::1/48";
  config.interfaces[1].intfID = kBaseVlanId + 1;
  config.interfaces[1].vlanID = kBaseVlanId + 1;
  config.interfaces[1].routerID = 0;
  // Globally adminstered MAC
  config.interfaces[1].mac_ref().value_unchecked() = getLocalCpuMacStr();
  config.interfaces[1].__isset.mac = true;
  config.interfaces[1].ipAddresses.resize(2);
  config.interfaces[1].ipAddresses[0] = "2.2.2.2/24";
  config.interfaces[1].ipAddresses[1] = "2::1/48";
  return config;
}

cfg::SwitchConfig multiplePortSingleVlanConfig(
    const HwSwitch* hwSwitch,
    const std::vector<PortID>& ports) {
  std::map<PortID, VlanID> port2vlan;
  auto portItr = ports.begin();
  for (; portItr != ports.end(); portItr++) {
    port2vlan[*portItr] = kDefaultVlanId;
  }
  return genPortVlanCfg(hwSwitch, ports, port2vlan, std::vector<VlanID>({}));
}

} // namespace utility
} // namespace fboss
} // namespace facebook
