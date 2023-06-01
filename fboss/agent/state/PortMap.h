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
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;
class Port;

using PortMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using PortMapThriftType = std::map<int16_t, state::PortFields>;

class PortMap;
using PortMapTraits =
    ThriftMapNodeTraits<PortMap, PortMapTypeClass, PortMapThriftType, Port>;

/*
 * A container for the set of ports.
 */
class PortMap : public ThriftMapNode<PortMap, PortMapTraits> {
 public:
  using Base = ThriftMapNode<PortMap, PortMapTraits>;
  using Traits = PortMapTraits;
  using Base::modify;

  PortMap();
  ~PortMap() override;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchPortMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, PortMapTypeClass>;
using MultiSwitchPortMapThriftType = std::map<std::string, PortMapThriftType>;

class MultiSwitchPortMap;

using MultiSwitchPortMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchPortMap,
    MultiSwitchPortMapTypeClass,
    MultiSwitchPortMapThriftType,
    PortMap>;

class HwSwitchMatcher;

class MultiSwitchPortMap : public ThriftMultiSwitchMapNode<
                               MultiSwitchPortMap,
                               MultiSwitchPortMapTraits> {
 public:
  using Traits = MultiSwitchPortMapTraits;
  using BaseT =
      ThriftMultiSwitchMapNode<MultiSwitchPortMap, MultiSwitchPortMapTraits>;
  using BaseT::modify;

  MultiSwitchPortMap* modify(std::shared_ptr<SwitchState>* state);
  std::shared_ptr<Port> getPort(const std::string& name) const;
  std::shared_ptr<Port> getPortIf(const std::string& name) const;

  MultiSwitchPortMap() {}
  virtual ~MultiSwitchPortMap() {}

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
