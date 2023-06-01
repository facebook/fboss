/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/VlanMap.h"

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"

#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NodeMap-defs.h"

#include <boost/container/flat_map.hpp>
#include <cstdio>

using std::shared_ptr;
using std::string;

namespace facebook::fboss {

VlanMap::VlanMap() {}

VlanMap::~VlanMap() {}

const shared_ptr<Vlan>& MultiSwitchVlanMap::getVlanSlow(
    const string& name) const {
  for (auto& iterMap : std::as_const(*this)) {
    for (auto& iter : std::as_const(*iterMap.second)) {
      const auto& vlan = iter.second;
      if (vlan->getName() == name) {
        return vlan;
      }
    }
  }
  throw FbossError("Cannot find Vlan : ", name);
}

shared_ptr<Vlan> MultiSwitchVlanMap::getVlanSlowIf(const string& name) const {
  for (auto& iterMap : std::as_const(*this)) {
    for (auto& iter : std::as_const(*iterMap.second)) {
      const auto& vlan = iter.second;
      if (vlan->getName() == name) {
        return vlan;
      }
    }
  }
  return nullptr;
}

MultiSwitchVlanMap* MultiSwitchVlanMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::vlanMaps>(state);
}

VlanID MultiSwitchVlanMap::getFirstVlanID() const {
  for (const auto& iter : std::as_const(*this)) {
    if (iter.second->size()) {
      return iter.second->cbegin()->second->getID();
    }
  }
  throw FbossError("No Vlans in MultiSwitchVlanMap");
}

template class ThriftMapNode<VlanMap, VlanMapTraits>;

} // namespace facebook::fboss
