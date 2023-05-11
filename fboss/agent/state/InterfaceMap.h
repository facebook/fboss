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

#include <folly/IPAddress.h>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class Interface;
class SwitchState;

using InterfaceMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using InterfaceMapThriftType = std::map<int32_t, state::InterfaceFields>;

class InterfaceMap;
using InterfaceMapTraits = ThriftMapNodeTraits<
    InterfaceMap,
    InterfaceMapTypeClass,
    InterfaceMapThriftType,
    Interface>;
/*
 * A container for the set of INTERFACEs.
 */
class InterfaceMap : public ThriftMapNode<InterfaceMap, InterfaceMapTraits> {
 public:
  using ThriftType = InterfaceMapThriftType;
  using Traits = InterfaceMapTraits;
  using Base = ThriftMapNode<InterfaceMap, InterfaceMapTraits>;
  using Base::modify;

  InterfaceMap();
  ~InterfaceMap() override;

  /*
   * Get the specified Interface.
   *
   * Throws an FbossError if the INTERFACE does not exist.
   */
  const std::shared_ptr<Interface> getInterface(InterfaceID id) const {
    return getNode(static_cast<int32_t>(id));
  }

  /*
   * Get the specified Interface.
   *
   * Returns null if the interface does not exist.
   */
  std::shared_ptr<Interface> getInterfaceIf(InterfaceID id) const {
    return getNodeIf(static_cast<int32_t>(id));
  }
  /*
   *  Get interface which has the given IPAddress. If multiple
   *  interfaces have the same address (unlikely) we return the
   *  first one. If no interface is found that has the given IP,
   *  we return null.
   */
  std::shared_ptr<Interface> getInterfaceIf(
      RouterID router,
      const folly::IPAddress& ip) const;

  /*
   * Same as get interface by IP above, but throws a execption
   * instead of returning null
   */
  const std::shared_ptr<Interface> getInterface(
      RouterID router,
      const folly::IPAddress& ip) const;

  typedef std::vector<std::shared_ptr<Interface>> Interfaces;

  /*
   *  Get interface which has the given vlan. If multiple
   *  interfaces exist, this will return the first interface.
   */
  std::shared_ptr<Interface> getInterfaceInVlanIf(VlanID vlan) const;

  /*
   * Same as getInterfaceInVlanIf but throws a execption
   * instead of returning null
   */
  const std::shared_ptr<Interface> getInterfaceInVlan(VlanID vlan) const;

  /*
   * Find an interface with its address to reach the given destination
   */
  const std::shared_ptr<Interface> getIntfToReach(
      RouterID router,
      const folly::IPAddress& dest) const;

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */

  void addInterface(const std::shared_ptr<Interface>& interface);
  void updateInterface(const std::shared_ptr<Interface>& interface);

  InterfaceMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchInterfaceMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, InterfaceMapTypeClass>;
using MultiSwitchInterfaceMapThriftType =
    std::map<std::string, InterfaceMapThriftType>;

class MultiSwitchInterfaceMap;

using MultiSwitchInterfaceMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchInterfaceMap,
    MultiSwitchInterfaceMapTypeClass,
    MultiSwitchInterfaceMapThriftType,
    InterfaceMap>;

class HwSwitchMatcher;

class MultiSwitchInterfaceMap : public ThriftMultiSwitchMapNode<
                                    MultiSwitchInterfaceMap,
                                    MultiSwitchInterfaceMapTraits> {
 public:
  using Traits = MultiSwitchInterfaceMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchInterfaceMap,
      MultiSwitchInterfaceMapTraits>;
  using BaseT::modify;

  MultiSwitchInterfaceMap() = default;
  virtual ~MultiSwitchInterfaceMap() = default;
  const std::shared_ptr<Interface> getIntfToReach(
      RouterID router,
      const folly::IPAddress& dest) const;

  MultiSwitchInterfaceMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
