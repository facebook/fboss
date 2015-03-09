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
#include <string>
#include <folly/Conv.h>
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/NodeMap-defs.h"

using std::string;
using folly::IPAddress;

namespace facebook { namespace fboss {

InterfaceMap::InterfaceMap() {
}

InterfaceMap::~InterfaceMap() {
}

std::shared_ptr<Interface>
InterfaceMap::getInterfaceIf(RouterID router, const IPAddress& ip) const {
  for (auto itr = begin(); itr != end(); ++itr) {
    if ((*itr)->getRouterID() == router && (*itr)->hasAddress(ip)) {
      return *itr;
    }
  }
  return nullptr;
}

const std::shared_ptr<Interface>&
InterfaceMap::getInterface(RouterID router, const IPAddress& ip) const {
  for (auto itr = begin(); itr != end(); ++itr) {
    if ((*itr)->getRouterID() == router && (*itr)->hasAddress(ip)) {
      return *itr;
    }
  }
  throw FbossError("No interface with ip : ", ip);
}

InterfaceMap::Interfaces
InterfaceMap::getInterfacesInVlanIf(VlanID vlan) const {
  Interfaces interfaces;
  for (auto itr = begin(); itr != end(); ++itr) {
    if ((*itr)->getVlanID() == vlan) {
      interfaces.push_back(*itr);
    }
  }
  return interfaces;
}

InterfaceMap::Interfaces
InterfaceMap::getInterfacesInVlan(VlanID vlan) const {
  auto interfaces = getInterfacesInVlanIf(vlan);
  if (interfaces.empty()) {
    throw FbossError("No interface in vlan : ", vlan);
  }
  return interfaces;
}

InterfaceMap::IntfAddrToReach InterfaceMap::getIntfAddrToReach(
    RouterID router, const folly::IPAddress& dest) const {
  for (auto iter = begin(); iter != end(); iter++) {
    const auto& intf = *iter;
    if (intf->getRouterID() == router) {
      auto addrIter = intf->getAddressToReach(dest);
      if (addrIter != intf->getAddresses().end()) {
        return IntfAddrToReach(intf.get(), &addrIter->first, addrIter->second);
      }
    }
  }
  return IntfAddrToReach(nullptr, nullptr, 0);
}

folly::dynamic InterfaceMap::toFollyDynamic() const {
  std::vector<folly::dynamic> intfs;
  for (const auto& intf: *this) {
    intfs.emplace_back(intf->toFollyDynamic());
  }
  return intfs;
}

std::shared_ptr<InterfaceMap>
InterfaceMap::fromFollyDynamic(const folly::dynamic& intfMapJson) {
  auto intfMap = std::make_shared<InterfaceMap>();
  for (const auto& intf: intfMapJson) {
    intfMap->addInterface(Interface::fromFollyDynamic(intf));
  }
  return intfMap;
}

FBOSS_INSTANTIATE_NODE_MAP(InterfaceMap, InterfaceMapTraits);

}} // facebook::fboss
