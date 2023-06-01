/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/gen-cpp2/switch_state_types.h"

#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

AggregatePortMap::AggregatePortMap() {}

AggregatePortMap::~AggregatePortMap() {}

std::shared_ptr<AggregatePort> AggregatePortMap::getAggregatePortForPortImpl(
    PortID port) const {
  for (const auto& idAndAggPort : std::as_const(*this)) {
    if (idAndAggPort.second->isMemberPort(port)) {
      return idAndAggPort.second;
    }
  }

  return nullptr;
}

MultiSwitchAggregatePortMap* MultiSwitchAggregatePortMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::aggregatePortMaps>(state);
}

std::shared_ptr<AggregatePort>
MultiSwitchAggregatePortMap::getAggregatePortForPort(PortID port) const {
  for (const auto& [_, aggPorts] : std::as_const(*this)) {
    if (auto aggPort = aggPorts->getAggregatePortForPort(port)) {
      return aggPort;
    }
  }
  return nullptr;
}

template class ThriftMapNode<AggregatePortMap, AggregatePortMapTraits>;

} // namespace facebook::fboss
