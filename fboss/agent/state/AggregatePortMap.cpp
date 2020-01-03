/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

AggregatePortMap::AggregatePortMap() {}

AggregatePortMap::~AggregatePortMap() {}

std::shared_ptr<AggregatePort> AggregatePortMap::getAggregatePortIf(
    PortID port) const {
  for (const auto& aggPort : *this) {
    if (aggPort->isMemberPort(port)) {
      return aggPort;
    }
  }

  return nullptr;
}

void AggregatePortMap::updateAggregatePort(
    const std::shared_ptr<AggregatePort>& aggPort) {
  updateNode(aggPort);
}

AggregatePortMap* AggregatePortMap::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newAggPorts = clone();
  auto* ptr = newAggPorts.get();
  (*state)->resetAggregatePorts(std::move(newAggPorts));
  return ptr;
}

FBOSS_INSTANTIATE_NODE_MAP(AggregatePortMap, AggregatePortMapTraits);

} // namespace facebook::fboss
