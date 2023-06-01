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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using SflowCollectorMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using SflowCollectorMapThriftType =
    std::map<std::string, state::SflowCollectorFields>;

class SflowCollectorMap;
using SflowCollectorMapTraits = ThriftMapNodeTraits<
    SflowCollectorMap,
    SflowCollectorMapTypeClass,
    SflowCollectorMapThriftType,
    SflowCollector>;
/*
 * A container for the set of collectors.
 */
class SflowCollectorMap
    : public ThriftMapNode<SflowCollectorMap, SflowCollectorMapTraits> {
 public:
  using Base = ThriftMapNode<SflowCollectorMap, SflowCollectorMapTraits>;
  using Traits = SflowCollectorMapTraits;
  SflowCollectorMap() = default;
  ~SflowCollectorMap() override = default;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchSflowCollectorMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, SflowCollectorMapTypeClass>;
using MultiSwitchSflowCollectorMapThriftType =
    std::map<std::string, SflowCollectorMapThriftType>;

class MultiSwitchSflowCollectorMap;

using MultiSwitchSflowCollectorMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchSflowCollectorMap,
    MultiSwitchSflowCollectorMapTypeClass,
    MultiSwitchSflowCollectorMapThriftType,
    SflowCollectorMap>;

class HwSwitchMatcher;

class MultiSwitchSflowCollectorMap : public ThriftMultiSwitchMapNode<
                                         MultiSwitchSflowCollectorMap,
                                         MultiSwitchSflowCollectorMapTraits> {
 public:
  using Traits = MultiSwitchSflowCollectorMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchSflowCollectorMap,
      MultiSwitchSflowCollectorMapTraits>;
  using BaseT::modify;

  MultiSwitchSflowCollectorMap() {}
  virtual ~MultiSwitchSflowCollectorMap() {}
  MultiSwitchSflowCollectorMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
