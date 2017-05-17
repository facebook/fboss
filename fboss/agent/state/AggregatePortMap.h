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

namespace facebook { namespace fboss {

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

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

}} // facebook::fboss
