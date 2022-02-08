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
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"

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
  void forEachChild(Fn /*fn*/) {}

  Table table;
};

} // namespace facebook::fboss
