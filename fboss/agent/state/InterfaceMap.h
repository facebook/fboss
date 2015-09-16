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
#include <vector>
#include <folly/IPAddress.h>
#include "fboss/agent/types.h"
#include "fboss/agent/state/NodeMap.h"
namespace facebook { namespace fboss {

class Interface;
typedef NodeMapTraits<InterfaceID, Interface> InterfaceMapTraits;

/*
 * A container for the set of INTERFACEs.
 */
class InterfaceMap : public NodeMapT<InterfaceMap, InterfaceMapTraits> {
 public:
  InterfaceMap();
  ~InterfaceMap() override;

  /*
   * Get the specified Interface.
   *
   * Throws an FbossError if the INTERFACE does not exist.
   */
  const std::shared_ptr<Interface>& getInterface(InterfaceID id) const {
    return getNode(id);
  }

  /*
   * Get the specified Interface.
   *
   * Returns null if the interface does not exist.
   */
  std::shared_ptr<Interface> getInterfaceIf(InterfaceID id) const {
    return getNodeIf(id);
  }
  /*
   *  Get interface which has the given IPAddress. If multiple
   *  interfaces have the same address (unlikely) we return the
   *  first one. If no interface is found that has the given IP,
   *  we return null.
   */
  std::shared_ptr<Interface> getInterfaceIf(
      RouterID router, const folly::IPAddress& ip) const;

  /*
   * Same as get interface by IP above, but throws a execption
   * instead of returning null
   */
  const std::shared_ptr<Interface>& getInterface(
      RouterID router, const folly::IPAddress& ip) const;

  typedef std::vector<std::shared_ptr<Interface>> Interfaces;
  /*
   * Get interfaces in a vlan
   */
  Interfaces getInterfacesInVlan(VlanID vlan) const;
  /*
   * Get interfaces in a vlan. Throw a exception if no interface is found
   */
  Interfaces getInterfacesInVlanIf(VlanID vlan) const;

  struct IntfAddrToReach {
    IntfAddrToReach(const Interface* intf, const folly::IPAddress *addr,
                    uint8_t mask) : intf(intf), addr(addr), mask(mask) {}
    const Interface *intf{nullptr};
    const folly::IPAddress *addr{nullptr};
    uint8_t mask{0};
  };
  /**
   * Find an interface with its address to reach the given destination
   */
  IntfAddrToReach getIntfAddrToReach(
      RouterID router, const folly::IPAddress& dest) const;

  /*
   * The following functions modify the static state.
   * These should only be called on unpublished objects which are only visible
   * to a single thread.
   */


  void addInterface(const std::shared_ptr<Interface>& interface) {
    addNode(interface);
  }

  /*
   * Serialize to a folly::dynamic object
   */
  folly::dynamic toFollyDynamic() const override;
  /*
   * Deserialize from a folly::dynamic object
   */
  static std::shared_ptr<InterfaceMap>
    fromFollyDynamic(const folly::dynamic& intfMapJson);

  static std::shared_ptr<InterfaceMap>
  fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

}} // facebook::fboss
