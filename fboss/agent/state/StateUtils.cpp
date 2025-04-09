/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

namespace {
const std::string kTunIntfPrefix = "fboss";
} // anonymous namespace

namespace facebook::fboss::utility {

bool isTunIntfName(std::string const& ifName) {
  return ifName.find(kTunIntfPrefix) == 0;
}

std::string createTunIntfName(InterfaceID ifID) {
  return folly::sformat("{}{}", kTunIntfPrefix, folly::to<std::string>(ifID));
}

InterfaceID getIDFromTunIntfName(std::string const& ifName) {
  if (not isTunIntfName(ifName)) {
    throw FbossError(ifName, " is not a valid tun interface");
  }

  return InterfaceID(atoi(ifName.substr(kTunIntfPrefix.size()).c_str()));
}

folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan) {
  return state->getInterfaces()->getInterfaceInVlan(vlan)->getMac();
}
folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intf) {
  return state->getInterfaces()->getNode(intf)->getMac();
}

folly::MacAddress getMacForFirstInterfaceWithPorts(
    const std::shared_ptr<SwitchState>& state) {
  auto intfID = firstInterfaceIDWithPorts(state);
  return getInterfaceMac(state, intfID);
}

std::optional<VlanID> firstVlanIDWithPorts(
    const std::shared_ptr<SwitchState>& state) {
  std::optional<VlanID> firstVlanId;
  auto intfID = firstInterfaceIDWithPorts(state);
  auto intf = state->getInterfaces()->getNode(intfID);
  if (intf->getType() == cfg::InterfaceType::VLAN) {
    firstVlanId = intf->getVlanID();
  }
  return firstVlanId;
}

InterfaceID firstInterfaceIDWithPorts(
    const std::shared_ptr<SwitchState>& state) {
  const auto& intfMap = state->getInterfaces()->cbegin()->second;
  for (const auto& [intfID, intf] : std::as_const(*intfMap)) {
    if (intf->isVirtual()) {
      // virtual interfaces do not have associated ports
      continue;
    }
    return InterfaceID(intfID);
  }
  throw FbossError("No interface found in state");
}

std::vector<folly::IPAddress> getIntfAddrs(
    const std::shared_ptr<SwitchState>& state,
    const InterfaceID& intf) {
  std::vector<folly::IPAddress> addrs;
  auto intfNode = state->getInterfaces()->getNode(intf);
  for (const auto& addr : std::as_const(*intfNode->getAddresses())) {
    addrs.emplace_back(addr.first);
  }
  return addrs;
}

std::vector<folly::IPAddressV4> getIntfAddrsV4(
    const std::shared_ptr<SwitchState>& state,
    const InterfaceID& intf) {
  std::vector<folly::IPAddressV4> addrs;

  for (auto addr : getIntfAddrs(state, intf)) {
    if (addr.isV4()) {
      addrs.emplace_back(addr.asV4());
    }
  }
  return addrs;
}

std::vector<folly::IPAddressV6> getIntfAddrsV6(
    const std::shared_ptr<SwitchState>& state,
    const InterfaceID& intf) {
  std::vector<folly::IPAddressV6> addrs;

  for (auto addr : getIntfAddrs(state, intf)) {
    if (addr.isV6()) {
      addrs.emplace_back(addr.asV6());
    }
  }
  return addrs;
}

std::optional<VlanID> getFirstVlanIDForTx_DEPRECATED(
    const std::shared_ptr<SwitchState>& state) {
  // TODO: avoid using getFirstNodeIf
  auto settings = utility::getFirstNodeIf(state->getSwitchSettings());
  auto l3SwitchType = settings->l3SwitchType();
  if (l3SwitchType == cfg::SwitchType::VOQ) {
    // no vlan for voq switch
    return std::nullopt;
  }
  const auto& switchInfo = settings->getSwitchInfo();
  auto asicType = switchInfo.asicType().value();
  auto vlan = firstVlanIDWithPorts(state);
  // TODO:  avoid using asicType
  if (vlan.has_value() || asicType != cfg::AsicType::ASIC_TYPE_CHENAB) {
    return vlan;
  }
  auto defaultVlan = settings->getDefaultVlan();
  if (!defaultVlan) {
    throw FbossError("no vlan found for tx");
  }
  return VlanID(*defaultVlan);
}

std::shared_ptr<Interface> firstInterfaceWithPorts(
    const std::shared_ptr<SwitchState>& state) {
  auto intfID = utility::firstInterfaceIDWithPorts(state);
  return state->getInterfaces()->getNodeIf(intfID);
}
} // namespace facebook::fboss::utility
