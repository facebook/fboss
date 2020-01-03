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
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/PortDescriptor.h"

namespace {
constexpr auto kNPending = "numPendingEntries";
}

namespace facebook::fboss {

class SwitchState;
class Vlan;

template <typename IPADDR, typename ENTRY>
struct NeighborTableTraits {
  typedef IPADDR KeyType;
  typedef ENTRY Node;
  typedef NodeMapNoExtraFields ExtraFields;

  static KeyType getKey(const std::shared_ptr<Node>& entry) {
    return entry->getIP();
  }
};

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
    : public NodeMapT<SUBCLASS, NeighborTableTraits<IPADDR, ENTRY>> {
 public:
  typedef IPADDR AddressType;
  typedef ENTRY Entry;

  NeighborTable();

  const std::shared_ptr<Entry>& getEntry(AddressType ip) const {
    return this->getNode(ip);
  }
  std::shared_ptr<Entry> getEntryIf(AddressType ip) const {
    return this->getNodeIf(ip);
  }

  SUBCLASS* modify(Vlan** vlan, std::shared_ptr<SwitchState>* state);

  /**
   * Return a modifiable version of current table. If the table is cloned, all
   * nodes to the root are cloned, and cloned state is put in the output
   * parameter.
   */
  SUBCLASS* modify(VlanID vlanId, std::shared_ptr<SwitchState>* state);

  void addEntry(
      AddressType ip,
      folly::MacAddress mac,
      PortDescriptor port,
      InterfaceID intfID,
      NeighborState state = NeighborState::REACHABLE);
  void addEntry(const NeighborEntryFields<AddressType>& fields);
  void updateEntry(
      AddressType ip,
      folly::MacAddress mac,
      PortDescriptor port,
      InterfaceID intfID,
      std::optional<cfg::AclLookupClass> classID = std::nullopt);
  void updateEntry(const NeighborEntryFields<AddressType>& fields);
  void updateEntry(AddressType ip, std::shared_ptr<ENTRY>);
  void addPendingEntry(AddressType ip, InterfaceID intfID);

  void removeEntry(AddressType ip);

 private:
  typedef NodeMapT<SUBCLASS, NeighborTableTraits<IPADDR, ENTRY>> Parent;
  // Inherit the constructors required for clone()
  using Parent::Parent;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
