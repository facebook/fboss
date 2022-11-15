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

void SystemPortMap::addSystemPort(
    const std::shared_ptr<SystemPort>& systemPort) {
  addNode(systemPort);
}

void SystemPortMap::updateSystemPort(
    const std::shared_ptr<SystemPort>& systemPort) {
  updateNode(systemPort);
}

void SystemPortMap::removeSystemPort(SystemPortID id) {
  removeNodeIf(id);
}

SystemPortMap* SystemPortMap::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  bool isRemote = (this == (*state)->getRemoteSystemPorts().get());
  SwitchState::modify(state);
  auto newSystemPorts = clone();
  auto* ptr = newSystemPorts.get();
  if (isRemote) {
    (*state)->resetRemoteSystemPorts(std::move(newSystemPorts));
  } else {
    (*state)->resetSystemPorts(std::move(newSystemPorts));
  }
  return ptr;
}

template class ThriftMapNode<SystemPortMap, SystemPortMapTraits>;
} // namespace facebook::fboss
