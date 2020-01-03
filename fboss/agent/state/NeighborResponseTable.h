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
#include <optional>
#include "fboss/agent/state/NodeBase.h"

#include <boost/container/flat_map.hpp>

namespace facebook::fboss {

struct NeighborResponseEntry {
  NeighborResponseEntry() {}
  NeighborResponseEntry(folly::MacAddress mac, InterfaceID interfaceID)
      : mac(mac), interfaceID(interfaceID) {}

  bool operator==(const NeighborResponseEntry& other) const {
    return (mac == other.mac && interfaceID == other.interfaceID);
  }
  bool operator!=(const NeighborResponseEntry& other) const {
    return !operator==(other);
  }
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;
  /*
   * Deserialize from folly::dynamic
   */
  static NeighborResponseEntry fromFollyDynamic(const folly::dynamic& entry);

  folly::MacAddress mac;
  InterfaceID interfaceID{0};
};

template <typename IPADDR>
struct NeighborResponseTableFields {
  typedef IPADDR AddressType;
  typedef boost::container::flat_map<AddressType, NeighborResponseEntry> Table;

  NeighborResponseTableFields() {}
  explicit NeighborResponseTableFields(Table&& t) : table(std::move(t)) {}
  NeighborResponseTableFields(
      const NeighborResponseTableFields& /*other*/,
      Table t)
      : table(std::move(t)) {}

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;
  /*
   * Deserialize from folly::dynamic
   */
  static NeighborResponseTableFields fromFollyDynamic(
      const folly::dynamic& entry);

  template <typename Fn>
  void forEachChild(Fn /*fn*/){};

  Table table;
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

} // namespace facebook::fboss
