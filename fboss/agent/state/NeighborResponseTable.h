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
#include "fboss/agent/state/NodeBase.h"

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
template <typename IPADDR, typename SUBCLASS>
class NeighborResponseTable
    : public NodeBaseT<SUBCLASS, NeighborResponseTableFields<IPADDR>> {
 public:
  typedef IPADDR AddressType;
  typedef typename NeighborResponseTableFields<AddressType>::Table Table;

  NeighborResponseTable() {}
  explicit NeighborResponseTable(Table table);

  static std::shared_ptr<SUBCLASS> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields =
        NeighborResponseTableFields<IPADDR>::fromFollyDynamic(json);
    return std::make_shared<SUBCLASS>(fields);
  }

  static std::shared_ptr<SUBCLASS> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return this->getFields()->toFollyDynamic();
  }

  std::optional<NeighborResponseEntry> getEntry(AddressType ip) {
    const auto& table = getTable();
    auto it = table.find(ip);
    if (it == table.end()) {
      return std::optional<NeighborResponseEntry>();
    }
    return it->second;
  }

  const Table& getTable() const {
    return this->getFields()->table;
  }
  void setTable(Table table) {
    this->writableFields()->table.swap(table);
  }

  /*
   * Set an entry in the table.
   *
   * This adds a new entry if this IP address is not already present in the
   * table, or updates the MAC address associated with this IP if it does
   * exist.
   */
  void setEntry(AddressType ip, folly::MacAddress mac, InterfaceID intfID) {
    auto& entry = this->writableFields()->table[ip];
    entry.mac = mac;
    entry.interfaceID = intfID;
  }

 private:
  // Inherit the constructors required for clone()
  typedef NodeBaseT<SUBCLASS, NeighborResponseTableFields<IPADDR>> Parent;
  using Parent::Parent;
  friend class CloneAllocator;
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
class NeighborResponseTableThrifty
    : public ThriftyNodeMapT<
          SUBCLASS,
          NeighborResponseTableTraits<IPADDR, ENTRY>,
          NeighborResponseTableThriftTraits<IPADDR, ENTRY>> {
 public:
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

 private:
  using Parent = ThriftyNodeMapT<
      SUBCLASS,
      NeighborResponseTableTraits<IPADDR, ENTRY>,
      NeighborResponseTableThriftTraits<IPADDR, ENTRY>>;
  // Inherit the constructors required for clone()
  using Parent::Parent;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
