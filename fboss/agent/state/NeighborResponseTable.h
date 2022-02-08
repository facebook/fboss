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

template <typename IPADDR, typename ENTRY>
struct NeighborResponseTableThriftTraits
    : public ThriftyNodeMapTraits<
          std::string,
          state::NeighborResponseEntryFields> {
  static inline const std::string& getThriftKeyName() {
    static const std::string _key = "ipAddress";
    return _key;
  }

  static const KeyType convertKey(const IPADDR& key) {
    return key.str();
  }

  static const KeyType parseKey(const folly::dynamic& key) {
    return key.asString();
  }
};

/*
 * A mapping of IPv4 --> MAC address, indicating how we should respond to ARP
 * requests on a given VLAN.
 *
 * This information is computed from the interface configuration, but is stored
 * with each VLAN so that we can efficiently respond to ARP requests.
 */
template <typename IPADDR, typename ENTRY, typename SUBCLASS>
class NeighborResponseTable
    : public ThriftyNodeMapT<
          SUBCLASS,
          NeighborResponseTableTraits<IPADDR, ENTRY>,
          NeighborResponseTableThriftTraits<IPADDR, ENTRY>> {
 public:
  typedef IPADDR AddressType;

  NeighborResponseTable() {}

  static folly::dynamic migrateToThrifty(const folly::dynamic& dyn) {
    folly::dynamic newItems = folly::dynamic::object;
    for (auto item : dyn.items()) {
      // inject key into node for ThriftyNodeMapT to find
      item.second[NeighborResponseTableThriftTraits<IPADDR, ENTRY>::
                      getThriftKeyName()] = item.first;
      newItems[item.first] = item.second;
    }
    return newItems;
  }

  static void migrateFromThrifty(folly::dynamic& /* dyn */) {}

  folly::dynamic toFollyDynamicLegacy() const {
    folly::dynamic entries = folly::dynamic::object;
    for (const auto& [ip, entry] : this->getAllNodes()) {
      entries[ip.str()] = entry->toFollyDynamicLegacy();
    }
    return entries;
  }

  static std::shared_ptr<SUBCLASS> fromFollyDynamicLegacy(
      const folly::dynamic& entries) {
    auto nbrTable = std::make_shared<SUBCLASS>();
    for (const auto& entry : entries.items()) {
      auto node = ENTRY::fromFollyDynamicLegacy(entry.second);
      // have to manually set IP because old dynamic struct did not include ip
      node->setIP(IPADDR(entry.first.stringPiece()));
      nbrTable->addNode(node);
    }
    return nbrTable;
  }

  std::shared_ptr<ENTRY> getEntry(AddressType ip) const {
    return this->getNodeIf(ip);
  }

  const typename NeighborResponseTableTraits<IPADDR, ENTRY>::NodeContainer&
  getTable() const {
    return this->getAllNodes();
  }

  /*
   * Set an entry in the table.
   *
   * This adds a new entry if this IP address is not already present in the
   * table, or updates the MAC address associated with this IP if it does
   * exist.
   */
  void setEntry(AddressType ip, folly::MacAddress mac, InterfaceID intfID) {
    auto& nodes = this->writableNodes();
    auto it = nodes.find(ip);
    if (it == nodes.end()) {
      this->addNode(std::make_shared<ENTRY>(ip, mac, intfID));
    } else {
      auto entry = it->second->clone();
      entry->setIP(ip);
      entry->setMac(mac);
      entry->setInterfaceID(intfID);
      it->second = entry;
    }
  }

 private:
  // Inherit the constructors required for clone()
  using Parent = ThriftyNodeMapT<
      SUBCLASS,
      NeighborResponseTableTraits<IPADDR, ENTRY>,
      NeighborResponseTableThriftTraits<IPADDR, ENTRY>>;
  using Parent::Parent;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
