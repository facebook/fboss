// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

using DsfNodeMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using DsfNodeMapThriftType = std::map<int64_t, cfg::DsfNode>;

class DsfNodeMap;
using DsfNodeMapTraits = ThriftMapNodeTraits<
    DsfNodeMap,
    DsfNodeMapTypeClass,
    DsfNodeMapThriftType,
    DsfNode>;

class DsfNodeMap : public ThriftMapNode<DsfNodeMap, DsfNodeMapTraits> {
 public:
  using Traits = DsfNodeMapTraits;
  using BaseT = ThriftMapNode<DsfNodeMap, DsfNodeMapTraits>;

  DsfNodeMap() = default;
  virtual ~DsfNodeMap() = default;

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

using MultiDsfNodeMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, DsfNodeMapTypeClass>;
using MultiDsfNodeMapThriftType = std::map<std::string, DsfNodeMapThriftType>;

class MultiDsfNodeMap;

using MultiDsfNodeMapTraits = ThriftMultiMapNodeTraits<
    MultiDsfNodeMap,
    MultiDsfNodeMapTypeClass,
    MultiDsfNodeMapThriftType,
    DsfNodeMap>;

class HwSwitchMatcher;

class MultiDsfNodeMap
    : public ThriftMultiMapNode<MultiDsfNodeMap, MultiDsfNodeMapTraits> {
 public:
  using Traits = MultiDsfNodeMapTraits;
  using BaseT = ThriftMultiMapNode<MultiDsfNodeMap, MultiDsfNodeMapTraits>;
  using BaseT::addNode;
  using BaseT::getNodeIf;
  using BaseT::modify;
  using BaseT::removeNode;
  using BaseT::updateNode;

  MultiDsfNodeMap() = default;
  virtual ~MultiDsfNodeMap() = default;

  MultiDsfNodeMap* modify(std::shared_ptr<SwitchState>* state);
  void addNode(
      std::shared_ptr<DsfNode> dsfNode,
      const HwSwitchMatcher& matcher);
  std::shared_ptr<DsfNode> getNodeIf(SwitchID switchID) const;
  void updateNode(
      std::shared_ptr<DsfNode> dsfNode,
      const HwSwitchMatcher& matcher);
  void removeNode(SwitchID switchId);
  size_t numNodes() const;

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
