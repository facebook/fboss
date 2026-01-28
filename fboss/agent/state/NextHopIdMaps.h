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

/*
 * IdToNextHopIdSetMap
 *
 * A map from NextHopSetId to a set of NextHopIds.
 * Since the value type is std::set<NextHopId> (not a struct),
 * we use thrift_cow::ThriftMapTraits directly as ThriftMapNodeTraits
 * is designed for struct values.
 */

using IdToNextHopIdSetMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::set<apache::thrift::type_class::integral>>;
using IdToNextHopIdSetMapThriftType =
    std::map<NextHopSetId, std::set<NextHopId>>;

class IdToNextHopIdSetMap;

using IdToNextHopIdSetMapTraits = thrift_cow::ThriftMapTraits<
    false,
    IdToNextHopIdSetMapTypeClass,
    IdToNextHopIdSetMapThriftType>;

class IdToNextHopIdSetMap : public thrift_cow::ThriftMapNode<
                                IdToNextHopIdSetMapTraits,
                                thrift_cow::TypeIdentity<IdToNextHopIdSetMap>> {
 public:
  using BaseT = thrift_cow::ThriftMapNode<
      IdToNextHopIdSetMapTraits,
      thrift_cow::TypeIdentity<IdToNextHopIdSetMap>>;
  using BaseT::BaseT;
  using Traits = IdToNextHopIdSetMapTraits;
  // Node is the ThriftSetNode type returned by the map iterator
  using Node = typename BaseT::Fields::value_type::element_type;

  IdToNextHopIdSetMap() = default;
  ~IdToNextHopIdSetMap() override = default;

  // Returns shared_ptr to the ThriftSetNode for the given NextHopSetId
  // Throws FbossError if not found
  const std::shared_ptr<Node>& getNextHopIdSet(NextHopSetId id) const {
    auto iter = this->find(id);
    if (iter == this->end()) {
      throw FbossError("NextHopIdSet with ID ", id, " not found");
    }
    return iter->second;
  }

  // Returns shared_ptr to the ThriftSetNode if found, nullptr otherwise
  const std::shared_ptr<Node> getNextHopIdSetIf(NextHopSetId id) const {
    auto iter = this->find(id);
    if (iter == this->end()) {
      return nullptr;
    }
    return iter->second;
  }

  void addNextHopIdSet(NextHopSetId id, const std::set<NextHopId>& nextHopIds) {
    this->emplace(id, nextHopIds);
  }

  void removeNextHopIdSet(NextHopSetId id) {
    if (!this->remove(id)) {
      throw FbossError("NextHopIdSet with ID ", id, " does not exist");
    }
  }

  bool removeNextHopIdSetIf(NextHopSetId id) {
    return this->remove(id);
  }

 private:
  friend class CloneAllocator;
};

} // namespace facebook::fboss
