/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <functional>
#include <memory>

#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/VlanMapDelta.h"
#include "fboss/agent/state/RouteDelta.h"

namespace facebook { namespace fboss {

class SwitchState;

/*
 * StateDelta contains code for examining the differences between two
 * SwitchStates.
 */
class StateDelta {
 public:
  StateDelta() {}
  StateDelta(std::shared_ptr<SwitchState> oldState,
             std::shared_ptr<SwitchState> newState)
    : old_(oldState),
      new_(newState) {}
  virtual ~StateDelta();

  const std::shared_ptr<SwitchState>& oldState() const {
    return old_;
  }
  const std::shared_ptr<SwitchState>& newState() const {
    return new_;
  }

  NodeMapDelta<PortMap> getPortsDelta() const;
  VlanMapDelta getVlansDelta() const;
  NodeMapDelta<InterfaceMap> getIntfsDelta() const;
  RTMapDelta getRouteTablesDelta() const;
  NodeMapDelta<AclMap> getAclsDelta() const;
  NodeMapDelta<AggregatePortMap> getAggregatePortsDelta() const;

 private:
  // Forbidden copy constructor and assignment operator
  StateDelta(StateDelta const &) = delete;
  StateDelta& operator=(StateDelta const &) = delete;

  std::shared_ptr<SwitchState> old_;
  std::shared_ptr<SwitchState> new_;
};

}} // facebook::fboss
