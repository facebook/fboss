// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/state/UdfGroup.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {
using UdfGroupMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using UdfGroupMapThriftType = std::map<std::string, cfg::UdfGroup>;

class UdfGroupMap;
using UdfGroupMapTraits = ThriftMapNodeTraits<
    UdfGroupMap,
    UdfGroupMapTypeClass,
    UdfGroupMapThriftType,
    UdfGroup>;

class UdfGroupMap : public ThriftMapNode<UdfGroupMap, UdfGroupMapTraits> {
 public:
  using BaseT = ThriftMapNode<UdfGroupMap, UdfGroupMapTraits>;
  UdfGroupMap() {}
  virtual ~UdfGroupMap() {}

  std::shared_ptr<UdfGroup> getUdfGroupIf(const std::string& name) const;
  void addUdfGroup(const std::shared_ptr<UdfGroup>& udfGroup);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
