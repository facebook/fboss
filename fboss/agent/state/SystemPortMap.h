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
#include "fboss/agent/state/SystemPort.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;

using SystemPortMapLegacyTraits = NodeMapTraits<SystemPortID, SystemPort>;

using SystemPortMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using SystemPortMapThriftType = std::map<int64_t, state::SystemPortFields>;

class SystemPortMap;
using SystemPortMapTraits = ThriftMapNodeTraits<
    SystemPortMap,
    SystemPortMapTypeClass,
    SystemPortMapThriftType,
    SystemPort>;

/*
 * A container for all the present SystemPorts
 */
class SystemPortMap : public ThriftMapNode<SystemPortMap, SystemPortMapTraits> {
 public:
  using Base = ThriftMapNode<SystemPortMap, SystemPortMapTraits>;
  using Traits = SystemPortMapTraits;
  using Base::modify;
  SystemPortMap();
  ~SystemPortMap() override;

  std::shared_ptr<SystemPort> getSystemPort(const std::string& name) const;
  std::shared_ptr<SystemPort> getSystemPortIf(const std::string& name) const;

  void addSystemPort(const std::shared_ptr<SystemPort>& systemPort);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
  bool isRemote_;
};

using MultiSwitchSystemPortMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, SystemPortMapTypeClass>;
using MultiSwitchSystemPortMapThriftType =
    std::map<std::string, SystemPortMapThriftType>;

class MultiSwitchSystemPortMap;

using MultiSwitchSystemPortMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchSystemPortMap,
    MultiSwitchSystemPortMapTypeClass,
    MultiSwitchSystemPortMapThriftType,
    SystemPortMap>;

class HwSwitchMatcher;

class MultiSwitchSystemPortMap : public ThriftMultiSwitchMapNode<
                                     MultiSwitchSystemPortMap,
                                     MultiSwitchSystemPortMapTraits> {
 public:
  using Traits = MultiSwitchSystemPortMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchSystemPortMap,
      MultiSwitchSystemPortMapTraits>;
  using BaseT::modify;

  MultiSwitchSystemPortMap() = default;
  virtual ~MultiSwitchSystemPortMap() = default;

  std::shared_ptr<SystemPort> getSystemPort(const std::string& name) const;
  std::shared_ptr<SystemPort> getSystemPortIf(const std::string& name) const;
  MultiSwitchSystemPortMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
