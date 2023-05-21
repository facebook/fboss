/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/PortMap.h"

#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

PortMap::PortMap() {}

PortMap::~PortMap() {}

std::shared_ptr<Port> MultiSwitchPortMap::getPort(
    const std::string& name) const {
  auto port = getPortIf(name);
  if (!port) {
    throw FbossError("Port with name: ", name, " not found");
  }
  return port;
}

std::shared_ptr<Port> MultiSwitchPortMap::getPortIf(
    const std::string& name) const {
  for (auto portMap : std::as_const(*this)) {
    for (auto port : std::as_const(*portMap.second)) {
      if (name == port.second->getName()) {
        return port.second;
      }
    }
  }
  return nullptr;
}

MultiSwitchPortMap* MultiSwitchPortMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::portMaps>(state);
}

template class ThriftMapNode<PortMap, PortMapTraits>;

} // namespace facebook::fboss
