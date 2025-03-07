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
#include "fboss/agent/FbossError.h"
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

folly::MacAddress getFirstInterfaceMac(
    const std::shared_ptr<SwitchState>& state) {
  auto intfID = firstInterfaceID(state);
  return getInterfaceMac(state, intfID);
}

std::optional<VlanID> firstVlanID(const std::shared_ptr<SwitchState>& state) {
  std::optional<VlanID> firstVlanId;
  auto intfID = firstInterfaceID(state);
  auto intf = state->getInterfaces()->getNode(intfID);
  if (intf->getType() == cfg::InterfaceType::VLAN) {
    firstVlanId = intf->getVlanID();
  }
  return firstVlanId;
}

InterfaceID firstInterfaceID(const std::shared_ptr<SwitchState>& state) {
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
} // namespace facebook::fboss::utility
