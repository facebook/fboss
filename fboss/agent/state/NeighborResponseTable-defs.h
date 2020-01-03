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

#include "fboss/agent/state/NodeBase-defs.h"

namespace facebook::fboss {

template <typename IPADDR>
folly::dynamic NeighborResponseTableFields<IPADDR>::toFollyDynamic() const {
  folly::dynamic entries = folly::dynamic::object;
  for (const auto& ipAndEntry : table) {
    folly::dynamic entry = folly::dynamic::object;
    entries[ipAndEntry.first.str()] = ipAndEntry.second.toFollyDynamic();
  }
  return entries;
}

template <typename IPADDR>
NeighborResponseTableFields<IPADDR>
NeighborResponseTableFields<IPADDR>::fromFollyDynamic(
    const folly::dynamic& entries) {
  NeighborResponseTableFields nbrTable;
  for (const auto& entry : entries.items()) {
    nbrTable.table.emplace(
        IPADDR(entry.first.stringPiece()),
        NeighborResponseEntry::fromFollyDynamic(entry.second));
  }
  return nbrTable;
}

template <typename IPADDR, typename SUBCLASS>
NeighborResponseTable<IPADDR, SUBCLASS>::NeighborResponseTable(Table table)
    : Parent(std::move(table)) {}

} // namespace facebook::fboss
