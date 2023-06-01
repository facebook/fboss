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

using MultiSwitchDsfNodeMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, DsfNodeMapTypeClass>;
using MultiSwitchDsfNodeMapThriftType =
    std::map<std::string, DsfNodeMapThriftType>;

class MultiSwitchDsfNodeMap;

using MultiSwitchDsfNodeMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchDsfNodeMap,
    MultiSwitchDsfNodeMapTypeClass,
    MultiSwitchDsfNodeMapThriftType,
    DsfNodeMap>;

class HwSwitchMatcher;

class MultiSwitchDsfNodeMap : public ThriftMultiSwitchMapNode<
                                  MultiSwitchDsfNodeMap,
                                  MultiSwitchDsfNodeMapTraits> {
 public:
  using Traits = MultiSwitchDsfNodeMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchDsfNodeMap,
      MultiSwitchDsfNodeMapTraits>;

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
