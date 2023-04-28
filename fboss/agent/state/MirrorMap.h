// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

using MirrorMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using MirrorMapThriftType = std::map<std::string, state::MirrorFields>;

class MirrorMap;
using MirrorMapTraits = ThriftMapNodeTraits<
    MirrorMap,
    MirrorMapTypeClass,
    MirrorMapThriftType,
    Mirror>;

class MirrorMap : public ThriftMapNode<MirrorMap, MirrorMapTraits> {
 public:
  using Traits = MirrorMapTraits;
  using BaseT = ThriftMapNode<MirrorMap, MirrorMapTraits>;
  MirrorMap() {}
  virtual ~MirrorMap() {}

  static std::shared_ptr<MirrorMap> fromThrift(
      const std::map<std::string, state::MirrorFields>& mirrors);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

using MultiMirrorMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, MirrorMapTypeClass>;
using MultiMirrorMapThriftType = std::map<std::string, MirrorMapThriftType>;

class MultiMirrorMap;

using MultiMirrorMapTraits = ThriftMultiMapNodeTraits<
    MultiMirrorMap,
    MultiMirrorMapTypeClass,
    MultiMirrorMapThriftType,
    MirrorMap>;

class HwSwitchMatcher;

class MultiMirrorMap
    : public ThriftMultiMapNode<MultiMirrorMap, MultiMirrorMapTraits> {
 public:
  using Traits = MultiMirrorMapTraits;
  using BaseT = ThriftMultiMapNode<MultiMirrorMap, MultiMirrorMapTraits>;
  using BaseT::modify;

  MultiMirrorMap* modify(std::shared_ptr<SwitchState>* state);
  static std::shared_ptr<MultiMirrorMap> fromThrift(
      const std::map<std::string, std::map<std::string, state::MirrorFields>>&
          mnpuMirrors);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
