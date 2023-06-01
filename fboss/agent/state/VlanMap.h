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

#include <string>
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;
class Vlan;

using VlanMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using VlanMapThriftType = std::map<int16_t, state::VlanFields>;

class VlanMap;
using VlanMapTraits =
    ThriftMapNodeTraits<VlanMap, VlanMapTypeClass, VlanMapThriftType, Vlan>;

/*
 * A container for the set of VLANs.
 */
class VlanMap : public ThriftMapNode<VlanMap, VlanMapTraits> {
 public:
  using Base = ThriftMapNode<VlanMap, VlanMapTraits>;
  using Traits = VlanMapTraits;
  using Base::modify;

  VlanMap();
  ~VlanMap() override;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchVlanMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, VlanMapTypeClass>;
using MultiSwitchVlanMapThriftType = std::map<std::string, VlanMapThriftType>;

class MultiSwitchVlanMap;

using MultiSwitchVlanMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchVlanMap,
    MultiSwitchVlanMapTypeClass,
    MultiSwitchVlanMapThriftType,
    VlanMap>;

class HwSwitchMatcher;

class MultiSwitchVlanMap : public ThriftMultiSwitchMapNode<
                               MultiSwitchVlanMap,
                               MultiSwitchVlanMapTraits> {
 public:
  using Traits = MultiSwitchVlanMapTraits;
  using BaseT =
      ThriftMultiSwitchMapNode<MultiSwitchVlanMap, MultiSwitchVlanMapTraits>;
  using BaseT::modify;

  MultiSwitchVlanMap* modify(std::shared_ptr<SwitchState>* state);

  /*
   * Get the specified Vlan by name. Throw FbossError if Vlan
   * does not exist.
   * Note that this requires a linear traversal of all Vlans
   *
   * TODO: Maintain a second index by name. Typically we have only a
   * handful of Vlans, so this traversal should be fairly cheap. We
   * may optimize it further in the future though.
   */
  const std::shared_ptr<Vlan>& getVlanSlow(const std::string& name) const;

  /*
   * Get the specified Vlan by name. Returns null if Vlan
   * does not exist.
   * Note that this requires a linear traversal of all Vlans
   *
   * TODO: Maintain a second index by name. Typically we have only a
   * handful of Vlans, so this traversal should be fairly cheap. We
   * may optimize it further in the future though.
   */
  std::shared_ptr<Vlan> getVlanSlowIf(const std::string& name) const;

  VlanID getFirstVlanID() const;

  MultiSwitchVlanMap() {}
  virtual ~MultiSwitchVlanMap() {}

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
