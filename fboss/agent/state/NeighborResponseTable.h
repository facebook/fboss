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

#include <fboss/agent/state/Thrifty.h>
#include <folly/MacAddress.h>
#include <memory>
#include <optional>
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NeighborResponseEntry.h"
#include "fboss/agent/state/NodeMap.h"

namespace facebook::fboss {

template <typename IPADDR, typename ENTRY>
struct NeighborResponseTableTraits {
  using KeyType = IPADDR;
  using Node = ENTRY;
  using ExtraFields = NodeMapNoExtraFields;
  using NodeContainer = std::map<KeyType, std::shared_ptr<Node>>;

  static KeyType getKey(const std::shared_ptr<Node>& entry) {
    return entry->getIP();
  }
};

using NbrResponseTableTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using NbrResponseTableThriftType =
    std::map<std::string, state::NeighborResponseEntryFields>;

template <typename SUBCLASS, typename NODE>
struct NbrResponseTableTraits : ThriftMapNodeTraits<
                                    SUBCLASS,
                                    NbrResponseTableTypeClass,
                                    NbrResponseTableThriftType,
                                    NODE> {};

/*
 * A mapping of IPv4 --> MAC address, indicating how we should respond to ARP
 * requests on a given VLAN.
 *
 * This information is computed from the interface configuration, but is stored
 * with each VLAN so that we can efficiently respond to ARP requests.
 */
template <typename IPADDR, typename ENTRY, typename SUBCLASS>
class NeighborResponseTable
    : public ThriftMapNode<SUBCLASS, NbrResponseTableTraits<SUBCLASS, ENTRY>> {
 public:
  typedef IPADDR AddressType;

  NeighborResponseTable() {}

  std::shared_ptr<ENTRY> getEntry(AddressType ip) const {
    return this->getNodeIf(ip.str());
  }

  /*
   * Set an entry in the table.
   *
   * This adds a new entry if this IP address is not already present in the
   * table, or updates the MAC address associated with this IP if it does
   * exist.
   */
  void setEntry(AddressType ip, folly::MacAddress mac, InterfaceID intfID) {
    auto entry = this->getEntry(ip);
    if (!entry) {
      this->addNode(std::make_shared<ENTRY>(ip, mac, intfID));
      return;
    }
    entry = entry->clone();
    entry->setIP(ip);
    entry->setMac(mac);
    entry->setInterfaceID(intfID);
    this->updateNode(entry);
  }

 private:
  // Inherit the constructors required for clone()
  using Parent =
      ThriftMapNode<SUBCLASS, NbrResponseTableTraits<SUBCLASS, ENTRY>>;
  using Parent::Parent;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
