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

#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;
class SaiNextHop;
class SaiFdbEntry;

class SaiNeighbor {
 public:
  SaiNeighbor(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      const NeighborApiParameters::EntryType& entry,
      const NeighborApiParameters::Attributes& attributes,
      std::unique_ptr<SaiFdbEntry> fdbEntry);
  ~SaiNeighbor();
  SaiNeighbor(const SaiNeighbor& other) = delete;
  SaiNeighbor(SaiNeighbor&& other) = delete;
  SaiNeighbor& operator=(const SaiNeighbor& other) = delete;
  SaiNeighbor& operator=(SaiNeighbor&& other) = delete;
  bool operator==(const SaiNeighbor& other) const;
  bool operator!=(const SaiNeighbor& other) const;

  const NeighborApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t nextHopId() const;

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  NeighborApiParameters::EntryType entry_;
  NeighborApiParameters::Attributes attributes_;
  std::unique_ptr<SaiNextHop> nextHop_;
  std::unique_ptr<SaiFdbEntry> fdbEntry_;
};

class SaiNeighborManager {
 public:
  SaiNeighborManager(SaiApiTable* apiTable, SaiManagerTable* managerTable);

  // Helper function to create a SAI NeighborEntry from an FBOSS SwitchState
  // NeighborEntry (e.g., NeighborEntry<IPAddressV6, NDPTable>)
  template <typename NeighborEntryT>
  NeighborApiParameters::EntryType saiEntryFromSwEntry(
      const std::shared_ptr<NeighborEntryT>& swEntry);

  template <typename NeighborEntryT>
  void changeNeighbor(
      const std::shared_ptr<NeighborEntryT>& oldSwEntry,
      const std::shared_ptr<NeighborEntryT>& newSwEntry);

  template <typename NeighborEntryT>
  void addNeighbor(const std::shared_ptr<NeighborEntryT>& swEntry);

  template <typename NeighborEntryT>
  void removeNeighbor(const std::shared_ptr<NeighborEntryT>& swEntry);

  SaiNeighbor* getNeighbor(const NeighborApiParameters::EntryType& entry);
  const SaiNeighbor* getNeighbor(
      const NeighborApiParameters::EntryType& entry) const;

  void processNeighborDelta(const StateDelta& delta);
  void clear();

 private:
  SaiNeighbor* getNeighborImpl(
      const NeighborApiParameters::EntryType& entry) const;
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  std::unordered_set<NeighborApiParameters::EntryType> unresolvedNeighbors_;
  std::unordered_map<
      NeighborApiParameters::EntryType,
      std::unique_ptr<SaiNeighbor>>
      neighbors_;
};

} // namespace fboss
} // namespace facebook
