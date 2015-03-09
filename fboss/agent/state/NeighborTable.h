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
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/NodeMap.h"
#include <folly/dynamic.h>
#include <folly/json.h>

namespace {
constexpr auto kPendingEntries = "hasPendingEntries";
}

namespace facebook { namespace fboss {

class SwitchState;
class Vlan;

struct NeighborTableFields {
  template<typename Fn> void forEachChild(Fn fn) {}
  bool pendingEntries{false};

  folly::dynamic toFollyDynamic() const {
    folly::dynamic ntable = folly::dynamic::object;
    ntable[kPendingEntries] = pendingEntries;
    return ntable;
  }

  static NeighborTableFields
  fromFollyDynamic(const folly::dynamic& ntableJson) {
    NeighborTableFields ntable;
    ntable.pendingEntries = ntableJson[kPendingEntries].asBool();
    // TODO(aeckert): t5833509 Add a check that verifies that the pending entry
    // flag is consistent with the entries

    return ntable;
  }

};

template<typename IPADDR, typename ENTRY>
struct NeighborTableTraits {
  typedef IPADDR KeyType;
  typedef ENTRY Node;
  typedef NeighborTableFields ExtraFields;

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
template<typename IPADDR, typename ENTRY, typename SUBCLASS>
class NeighborTable
  : public NodeMapT<SUBCLASS, NeighborTableTraits<IPADDR, ENTRY>> {
 public:
  typedef IPADDR AddressType;
  typedef ENTRY Entry;

  NeighborTable();

  std::shared_ptr<Entry> getEntry(AddressType ip) const {
    return this->getNode(ip);
  }
  std::shared_ptr<Entry> getEntryIf(AddressType ip) const {
    return this->getNodeIf(ip);
  }

  SUBCLASS* modify(Vlan** vlan, std::shared_ptr<SwitchState>* state);

  void addEntry(AddressType ip,
                folly::MacAddress mac,
                PortID port,
                InterfaceID intfID);
  void updateEntry(AddressType ip,
                   folly::MacAddress mac,
                   PortID port,
                   InterfaceID intfID);
  void addPendingEntry(AddressType ip,
                       InterfaceID intfID);

  bool prunePendingEntries();

  bool hasPendingEntries() {
    return this->getExtraFields().pendingEntries;
  }

 private:
  typedef NodeMapT<SUBCLASS, NeighborTableTraits<IPADDR, ENTRY>> Parent;
  // Inherit the constructors required for clone()
  using Parent::NodeMapT;
  friend class CloneAllocator;

  void setPendingEntries(bool pendingEntries) {
    this->writableExtraFields().pendingEntries = pendingEntries;
  }

};

}} // facebook::fboss
