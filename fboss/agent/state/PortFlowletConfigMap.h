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
#include "fboss/agent/state/PortFlowletConfig.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using PortFlowletCfgMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using PortFlowletCfgMapThriftType =
    std::map<std::string, state::PortFlowletFields>;

class PortFlowletCfgMap;
using PortFlowletCfgMapTraits = ThriftMapNodeTraits<
    PortFlowletCfgMap,
    PortFlowletCfgMapTypeClass,
    PortFlowletCfgMapThriftType,
    PortFlowletCfg>;

/*
 * A container for the set of collectors.
 */
class PortFlowletCfgMap
    : public ThriftMapNode<PortFlowletCfgMap, PortFlowletCfgMapTraits> {
 public:
  using Base = ThriftMapNode<PortFlowletCfgMap, PortFlowletCfgMapTraits>;
  using Traits = PortFlowletCfgMapTraits;
  PortFlowletCfgMap() {}
  virtual ~PortFlowletCfgMap() {}

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchPortFlowletCfgMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, PortFlowletCfgMapTypeClass>;
using MultiSwitchPortFlowletCfgMapThriftType =
    std::map<std::string, PortFlowletCfgMapThriftType>;

class MultiSwitchPortFlowletCfgMap;

using MultiSwitchPortFlowletCfgMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchPortFlowletCfgMap,
    MultiSwitchPortFlowletCfgMapTypeClass,
    MultiSwitchPortFlowletCfgMapThriftType,
    PortFlowletCfgMap>;

class HwSwitchMatcher;

class MultiSwitchPortFlowletCfgMap : public ThriftMultiSwitchMapNode<
                                         MultiSwitchPortFlowletCfgMap,
                                         MultiSwitchPortFlowletCfgMapTraits> {
 public:
  using Traits = MultiSwitchPortFlowletCfgMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchPortFlowletCfgMap,
      MultiSwitchPortFlowletCfgMapTraits>;
  using BaseT::modify;

  MultiSwitchPortFlowletCfgMap() {}
  virtual ~MultiSwitchPortFlowletCfgMap() {}

  MultiSwitchPortFlowletCfgMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
