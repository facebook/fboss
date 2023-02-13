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

#include <folly/MacAddress.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <type_traits>
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/PortDescriptor.h"

namespace facebook::fboss {

class SwitchState;
class Vlan;
class Interface;

template <typename IPADDR, typename ENTRY>
struct NeighborTableTraits {
  using KeyType = IPADDR;
  using Node = ENTRY;
  using ExtraFields = NodeMapNoExtraFields;
  using NodeContainer =
      boost::container::flat_map<KeyType, std::shared_ptr<Node>>;

  static KeyType getKey(const std::shared_ptr<Node>& entry) {
    return entry->getIP();
  }
};

using NbrTableTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using NbrTableThriftType = std::map<std::string, state::NeighborEntryFields>;

template <typename SUBCLASS, typename NODE>
struct NbrTableTraits : ThriftMapNodeTraits<
                            SUBCLASS,
                            NbrTableTypeClass,
                            NbrTableThriftType,
                            NODE> {};

/*
 * A map of IP --> MAC for the IP addresses of other nodes on a VLAN.
 *
 * TODO: We should switch from NodeMap to PrefixMap when the new PrefixMap
 * implementation is ready.
 *
 * Any change to a NodeMap is O(N), so it is really only suitable for small
 * maps that do not change frequently.  Our new PrefixMap implementation will
 * allow us to perform cheaper copy-on-write updates.
 */
template <typename IPADDR, typename ENTRY, typename SUBCLASS>
class NeighborTable
    : public ThriftMapNode<SUBCLASS, NbrTableTraits<SUBCLASS, ENTRY>> {
 public:
  typedef IPADDR AddressType;
  typedef ENTRY Entry;

  NeighborTable();

  const std::shared_ptr<Entry> getEntry(AddressType ip) const {
    static_assert(std::is_same_v<typename Parent::Node, ENTRY>, "Not Entry");
    return this->getNode(ip.str());
  }
  std::shared_ptr<Entry> getEntryIf(AddressType ip) const {
    return this->getNodeIf(ip.str());
  }
  /**
   * Return a modifiable version of current table. If the table is cloned, all
   * nodes to the root are cloned, and cloned state is put in the output
   * parameter.
   */

  SUBCLASS* modify(Vlan** vlan, std::shared_ptr<SwitchState>* state);
  SUBCLASS* modify(VlanID vlanId, std::shared_ptr<SwitchState>* state);
  SUBCLASS* modify(Interface** intf, std::shared_ptr<SwitchState>* state);
  SUBCLASS* modify(InterfaceID intfId, std::shared_ptr<SwitchState>* state);

  void addEntry(
      AddressType ip,
      folly::MacAddress mac,
      PortDescriptor port,
      InterfaceID intfID,
      NeighborState state = NeighborState::REACHABLE,
      std::optional<cfg::AclLookupClass> classID = std::nullopt,
      std::optional<int64_t> encapIndex = std::nullopt,
      bool isLocal = true);
  void addEntry(const NeighborEntryFields<AddressType>& fields);
  void updateEntry(
      AddressType ip,
      folly::MacAddress mac,
      PortDescriptor port,
      InterfaceID intfID,
      NeighborState state,
      std::optional<cfg::AclLookupClass> classID = std::nullopt,
      std::optional<int64_t> encapIndex = std::nullopt,
      bool isLocal = true);
  void updateEntry(const NeighborEntryFields<AddressType>& fields);
  void updateEntry(AddressType ip, std::shared_ptr<ENTRY>);
  void addPendingEntry(AddressType ip, InterfaceID intfID);

  void removeEntry(AddressType ip);

 private:
  using Parent = ThriftMapNode<SUBCLASS, NbrTableTraits<SUBCLASS, ENTRY>>;
  // Inherit the constructors required for clone()
  using Parent::Parent;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
