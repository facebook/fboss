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
  using BaseT = ThriftMapNode<DsfNodeMap, DsfNodeMapTraits>;
  using BaseT::modify;

  DsfNodeMap() {}
  virtual ~DsfNodeMap() {}

  DsfNodeMap* modify(std::shared_ptr<SwitchState>* state);
  std::shared_ptr<DsfNode> getDsfNodeIf(SwitchID switchId) const;

  void addDsfNode(const std::shared_ptr<DsfNode>& dsfNode);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
