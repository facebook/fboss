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

#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiNeighbor = SaiObject<SaiNeighborTraits>;

struct SaiNeighborHandle {
  std::shared_ptr<SaiNeighbor> neighbor;
  std::shared_ptr<SaiFdbEntry> fdbEntry;
  std::shared_ptr<SaiNextHop> nextHop;
};

class SaiNeighborManager {
 public:
  SaiNeighborManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);

  // Helper function to create a SAI NeighborEntry from an FBOSS SwitchState
  // NeighborEntry (e.g., NeighborEntry<IPAddressV6, NDPTable>)
  template <typename NeighborEntryT>
  SaiNeighborTraits::NeighborEntry saiEntryFromSwEntry(
      const std::shared_ptr<NeighborEntryT>& swEntry);

  template <typename NeighborEntryT>
  void changeNeighbor(
      const std::shared_ptr<NeighborEntryT>& oldSwEntry,
      const std::shared_ptr<NeighborEntryT>& newSwEntry);

  template <typename NeighborEntryT>
  void addNeighbor(const std::shared_ptr<NeighborEntryT>& swEntry);

  template <typename NeighborEntryT>
  void removeNeighbor(const std::shared_ptr<NeighborEntryT>& swEntry);

  SaiNeighborHandle* getNeighborHandle(
      const SaiNeighborTraits::NeighborEntry& entry);
  const SaiNeighborHandle* getNeighborHandle(
      const SaiNeighborTraits::NeighborEntry& entry) const;

  void processNeighborDelta(const StateDelta& delta);
  void clear();

 private:
  SaiNeighborHandle* getNeighborHandleImpl(
      const SaiNeighborTraits::NeighborEntry& entry) const;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  std::unordered_set<SaiNeighborTraits::NeighborEntry> unresolvedNeighbors_;
  folly::F14FastMap<
      SaiNeighborTraits::NeighborEntry,
      std::unique_ptr<SaiNeighborHandle>>
      handles_;
};

} // namespace facebook::fboss
