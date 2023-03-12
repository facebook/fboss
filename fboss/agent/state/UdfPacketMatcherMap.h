// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/state/UdfPacketMatcher.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {
using UdfPacketMatcherMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using UdfPacketMatcherMapThriftType =
    std::map<std::string, cfg::UdfPacketMatcher>;

class UdfPacketMatcherMap;
using UdfPacketMatcherMapTraits = ThriftMapNodeTraits<
    UdfPacketMatcherMap,
    UdfPacketMatcherMapTypeClass,
    UdfPacketMatcherMapThriftType,
    UdfPacketMatcher>;

class UdfPacketMatcherMap
    : public ThriftMapNode<UdfPacketMatcherMap, UdfPacketMatcherMapTraits> {
 public:
  using BaseT = ThriftMapNode<UdfPacketMatcherMap, UdfPacketMatcherMapTraits>;
  using BaseT::modify;

  UdfPacketMatcherMap() {}
  virtual ~UdfPacketMatcherMap() {}

  UdfPacketMatcherMap* modify(std::shared_ptr<SwitchState>* state);
  std::shared_ptr<UdfPacketMatcher> getUdfPacketMatcherIf(
      const std::string& name) const;
  void addUdfPacketMatcher(
      const std::shared_ptr<UdfPacketMatcher>& udfPacketMatcher);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
