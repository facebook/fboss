/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/SystemPortMap.h"

#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPort.h"

namespace facebook::fboss {

SystemPortMap::SystemPortMap() {}

SystemPortMap::~SystemPortMap() {}

std::shared_ptr<SystemPort> SystemPortMap::getSystemPort(
    const std::string& name) const {
  auto port = getSystemPortIf(name);
  if (!port) {
    throw FbossError("SystemPort with name: ", name, " not found");
  }
  return port;
}

std::shared_ptr<SystemPort> SystemPortMap::getSystemPortIf(
    const std::string& name) const {
  for (auto [id, sysPort] : *this) {
    if (name == sysPort->getPortName()) {
      return sysPort;
    }
  }
  return nullptr;
}

void SystemPortMap::addSystemPort(
    const std::shared_ptr<SystemPort>& systemPort) {
  addNode(systemPort);
}

std::shared_ptr<SystemPort> MultiSwitchSystemPortMap::getSystemPort(
    const std::string& name) const {
  auto port = getSystemPortIf(name);
  if (!port) {
    throw FbossError("SystemPort with name: ", name, " not found");
  }
  return port;
}

std::shared_ptr<SystemPort> MultiSwitchSystemPortMap::getSystemPortIf(
    const std::string& name) const {
  for (const auto& [_, map] : *this) {
    if (auto sysPort = map->getSystemPortIf(name)) {
      return sysPort;
    }
  }
  return nullptr;
}

MultiSwitchSystemPortMap* MultiSwitchSystemPortMap::modify(
    std::shared_ptr<SwitchState>* state) {
  bool isRemote = (this == (*state)->getRemoteSystemPorts().get());
  if (isRemote) {
    return SwitchState::modify<switch_state_tags::remoteSystemPortMaps>(state);
  } else {
    return SwitchState::modify<switch_state_tags::systemPortMaps>(state);
  }
}

template class ThriftMapNode<SystemPortMap, SystemPortMapTraits>;
} // namespace facebook::fboss
