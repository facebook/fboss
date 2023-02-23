/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/InterfaceMap.h"
#include <folly/Conv.h>
#include <string>
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

using folly::IPAddress;
using std::string;

namespace facebook::fboss {

InterfaceMap::InterfaceMap() {}

InterfaceMap::~InterfaceMap() {}

std::shared_ptr<Interface> InterfaceMap::getInterfaceIf(
    RouterID router,
    const IPAddress& ip) const {
  for (auto itr = begin(); itr != end(); ++itr) {
    auto intf = itr->second;
    if (intf->getRouterID() == router && intf->hasAddress(ip)) {
      return intf;
    }
  }
  return nullptr;
}

const std::shared_ptr<Interface> InterfaceMap::getInterface(
    RouterID router,
    const IPAddress& ip) const {
  for (auto itr = begin(); itr != end(); ++itr) {
    auto intf = itr->second;
    if (intf->getRouterID() == router && intf->hasAddress(ip)) {
      return intf;
    }
  }
  throw FbossError("No interface with ip : ", ip);
}

std::shared_ptr<Interface> InterfaceMap::getInterfaceInVlanIf(
    VlanID vlan) const {
  for (auto itr = begin(); itr != end(); ++itr) {
    auto intf = itr->second;
    if (intf->getVlanID() == vlan) {
      return intf;
    }
  }
  return nullptr;
}

const std::shared_ptr<Interface> InterfaceMap::getInterfaceInVlan(
    VlanID vlan) const {
  auto interface = getInterfaceInVlanIf(vlan);
  if (!interface) {
    throw FbossError("No interface in vlan : ", vlan);
  }
  return interface;
}

const std::shared_ptr<Interface> InterfaceMap::getIntfToReach(
    RouterID router,
    const folly::IPAddress& dest) const {
  for (auto itr : std::as_const(*this)) {
    const auto& intf = itr.second;
    if (intf->getRouterID() == router && intf->canReachAddress(dest)) {
      return intf;
    }
  }
  return nullptr;
}

void InterfaceMap::addInterface(const std::shared_ptr<Interface>& interface) {
  addNode(interface);
}

void InterfaceMap::updateInterface(
    const std::shared_ptr<Interface>& interface) {
  updateNode(interface);
}

InterfaceMap* InterfaceMap::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  bool isRemote = (this == (*state)->getRemoteInterfaces().get());
  SwitchState::modify(state);
  auto newInterfaces = clone();
  auto* ptr = newInterfaces.get();
  if (isRemote) {
    (*state)->resetRemoteIntfs(std::move(newInterfaces));
  } else {
    (*state)->resetIntfs(std::move(newInterfaces));
  }
  return ptr;
}

template class ThriftMapNode<InterfaceMap, InterfaceMapTraits>;

} // namespace facebook::fboss
