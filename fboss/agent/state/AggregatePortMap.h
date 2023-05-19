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
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"

namespace facebook::fboss {

class AggregatePort;

using AggregatePortMapLegacyTraits =
    NodeMapTraits<AggregatePortID, AggregatePort>;

using AggregatePortMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using AggregatePortMapThriftType =
    std::map<int16_t, state::AggregatePortFields>;

class AggregatePortMap;
using AggregatePortMapTraits = ThriftMapNodeTraits<
    AggregatePortMap,
    AggregatePortMapTypeClass,
    AggregatePortMapThriftType,
    AggregatePort>;

/*
 * A container for the set of aggregate ports.
 */
class AggregatePortMap
    : public ThriftMapNode<AggregatePortMap, AggregatePortMapTraits> {
 public:
  using Base = ThriftMapNode<AggregatePortMap, AggregatePortMapTraits>;
  using Traits = AggregatePortMapTraits;
  using Base::modify;
  using ThriftType = std::map<int16_t, state::AggregatePortFields>;

  AggregatePortMap();
  ~AggregatePortMap() override;

  AggregatePortMap* modify(std::shared_ptr<SwitchState>* state);

  std::shared_ptr<AggregatePort> getAggregatePortForPort(PortID port) const {
    return getAggregatePortForPortImpl(port);
  }

 private:
  /* This method will iterate over every member port in every aggregate port,
   * so it is a quadratic operation. If it turns out to be a bottleneck, we can
   * maintain an index to speed it up.
   */
  std::shared_ptr<AggregatePort> getAggregatePortForPortImpl(PortID port) const;

  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchAggregatePortMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, AggregatePortMapTypeClass>;
using MultiSwitchAggregatePortMapThriftType =
    std::map<std::string, AggregatePortMapThriftType>;

class MultiSwitchAggregatePortMap;

using MultiSwitchAggregatePortMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchAggregatePortMap,
    MultiSwitchAggregatePortMapTypeClass,
    MultiSwitchAggregatePortMapThriftType,
    AggregatePortMap>;

class HwSwitchMatcher;

class MultiSwitchAggregatePortMap : public ThriftMultiSwitchMapNode<
                                        MultiSwitchAggregatePortMap,
                                        MultiSwitchAggregatePortMapTraits> {
 public:
  using Traits = MultiSwitchAggregatePortMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchAggregatePortMap,
      MultiSwitchAggregatePortMapTraits>;
  using BaseT::modify;

  MultiSwitchAggregatePortMap() {}
  virtual ~MultiSwitchAggregatePortMap() {}

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
