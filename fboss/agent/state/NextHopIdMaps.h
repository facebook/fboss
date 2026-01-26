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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"

#include <memory>

namespace facebook::fboss {

class SwitchState;
class FibInfo;

using NextHopId = state::NextHopIdType;
using NextHopSetId = state::NextHopSetIdType;

/*
 * IdToNextHopMap
 *
 * A map from NextHopId to NextHopThrift.
 * This is used to determine Nexthop details for a given NextHopID.
 *
 */
using IdToNextHopMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using IdToNextHopMapThriftType = std::map<NextHopId, NextHopThrift>;

class IdToNextHopMap;

// Using ThriftMapNodeTraits with default Node type
using IdToNextHopMapTraits = ThriftMapNodeTraits<
    IdToNextHopMap,
    IdToNextHopMapTypeClass,
    IdToNextHopMapThriftType>;

class IdToNextHopMap
    : public ThriftMapNode<IdToNextHopMap, IdToNextHopMapTraits> {
 public:
  using BaseT = ThriftMapNode<IdToNextHopMap, IdToNextHopMapTraits>;
  using BaseT::modify;
  using Traits = IdToNextHopMapTraits;
  using Node = typename Traits::Node;

  IdToNextHopMap() = default;
  ~IdToNextHopMap() override = default;
  const std::shared_ptr<Node>& getNextHop(NextHopId id) const {
    return getNode(id);
  }

  std::shared_ptr<Node> getNextHopIf(NextHopId id) const {
    return getNodeIf(id);
  }

  void addNextHop(NextHopId id, const NextHopThrift& nextHop) {
    auto node = std::make_shared<Node>(nextHop);
    addNode(id, std::move(node));
  }

  void removeNextHop(NextHopId id) {
    removeNode(id);
  }

  std::shared_ptr<Node> removeNextHopIf(NextHopId id) {
    return removeNodeIf(id);
  }

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
