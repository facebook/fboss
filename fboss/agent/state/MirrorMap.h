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
  using BaseT::modify;
  MirrorMap() {}
  virtual ~MirrorMap() {}

  MirrorMap* modify(std::shared_ptr<SwitchState>* state);
  std::shared_ptr<Mirror> getMirrorIf(const std::string& name) const;

  void addMirror(const std::shared_ptr<Mirror>& mirror);

  static std::shared_ptr<MirrorMap> fromThrift(
      const std::map<std::string, state::MirrorFields>& mirrors) {
    auto map = std::make_shared<MirrorMap>();
    for (auto mirror : mirrors) {
      auto node = std::make_shared<Mirror>();
      node->fromThrift(mirror.second);
      // TODO(pshaikh): make this private
      node->markResolved();
      map->insert(*mirror.second.name(), std::move(node));
    }
    return map;
  }

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
    : public ThriftMapNode<MultiMirrorMap, MultiMirrorMapTraits> {
 public:
  using Traits = MultiMirrorMapTraits;
  using BaseT = ThriftMapNode<MultiMirrorMap, MultiMirrorMapTraits>;
  using BaseT::modify;

  MultiMirrorMap() {}
  virtual ~MultiMirrorMap() {}

  std::shared_ptr<const MirrorMap> getMirrorMapIf(
      const HwSwitchMatcher& matcher) const;

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
