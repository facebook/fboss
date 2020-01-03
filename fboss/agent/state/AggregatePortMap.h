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

#include "fboss/agent/state/NodeMap.h"

namespace facebook::fboss {

class AggregatePort;

using AggregatePortMapTraits = NodeMapTraits<AggregatePortID, AggregatePort>;

/*
 * A container for the set of aggregate ports.
 */
class AggregatePortMap
    : public NodeMapT<AggregatePortMap, AggregatePortMapTraits> {
 public:
  AggregatePortMap();
  ~AggregatePortMap() override;

  std::shared_ptr<AggregatePort> getAggregatePortIf(AggregatePortID id) const {
    return getNodeIf(id);
  }

  std::shared_ptr<AggregatePort> getAggregatePort(AggregatePortID id) const {
    return getNode(id);
  }

  /* This method will iterate over every member port in every aggregate port,
   * so it is a quadratic operation. If it turns out to be a bottleneck, we can
   * maintain an index to speed it up.
   */
  std::shared_ptr<AggregatePort> getAggregatePortIf(PortID port) const;

  void updateAggregatePort(const std::shared_ptr<AggregatePort>& aggPort);

  AggregatePortMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
