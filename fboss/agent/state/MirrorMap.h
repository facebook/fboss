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

using MultiSwitchMirrorMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, MirrorMapTypeClass>;
using MultiSwitchMirrorMapThriftType =
    std::map<std::string, MirrorMapThriftType>;

class MultiSwitchMirrorMap;

using MultiSwitchMirrorMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchMirrorMap,
    MultiSwitchMirrorMapTypeClass,
    MultiSwitchMirrorMapThriftType,
    MirrorMap>;

class HwSwitchMatcher;

class MultiSwitchMirrorMap : public ThriftMultiSwitchMapNode<
                                 MultiSwitchMirrorMap,
                                 MultiSwitchMirrorMapTraits> {
 public:
  using Traits = MultiSwitchMirrorMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchMirrorMap,
      MultiSwitchMirrorMapTraits>;
  using BaseT::modify;

  MultiSwitchMirrorMap* modify(std::shared_ptr<SwitchState>* state);
  static std::shared_ptr<MultiSwitchMirrorMap> fromThrift(
      const std::map<std::string, std::map<std::string, state::MirrorFields>>&
          mnpuMirrors);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
