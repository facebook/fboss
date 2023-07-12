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

#include "fboss/agent/types.h"

#include <folly/MacAddress.h>

#include <memory>

namespace facebook::fboss {
class SwitchState;
class StateDelta;
class MacEntry;
class ArpEntry;
class NdpEntry;

class StaticL2ForNeighborUpdater {
 public:
  explicit StaticL2ForNeighborUpdater(bool needL2EntryForNeighbor)
      : needL2EntryForNeighbor_(needL2EntryForNeighbor) {}
  virtual ~StaticL2ForNeighborUpdater() = default;

  void stateUpdated(const StateDelta& stateDelta);

  void processAdded(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<MacEntry>& macEntry);
  void processRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<MacEntry>& macEntry);
  void processChanged(
      const StateDelta& stateDelta,
      VlanID vlan,
      const std::shared_ptr<MacEntry>& oldEntry,
      const std::shared_ptr<MacEntry>& newEntry);
  template <typename NeighborEntryT>
  void processAdded(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<NeighborEntryT>& addedEntry);
  template <typename NeighborEntryT>
  void processRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<NeighborEntryT>& removedEntry);
  template <typename NeighborEntryT>
  void processChanged(
      const StateDelta& stateDelta,
      VlanID vlan,
      const std::shared_ptr<NeighborEntryT>& oldEntry,
      const std::shared_ptr<NeighborEntryT>& newEntry);

 private:
  virtual void ensureMacEntryForNeighbor(
      VlanID vlan,
      const std::shared_ptr<ArpEntry>& neighbor) = 0;
  virtual void ensureMacEntryForNeighbor(
      VlanID vlan,
      const std::shared_ptr<NdpEntry>& neighbor) = 0;

  virtual void pruneMacEntryForNeighbor(
      VlanID vlan,
      const std::shared_ptr<ArpEntry>& neighbor) = 0;
  virtual void pruneMacEntryForNeighbor(
      VlanID vlan,
      const std::shared_ptr<NdpEntry>& neighbor) = 0;

  virtual void ensureMacEntryIfNeighborExists(
      VlanID vlan,
      const std::shared_ptr<MacEntry>& macEntry) = 0;

  template <typename NeighborEntryT>
  void assertNeighborEntry(const NeighborEntryT& neighbor);
  bool needL2EntryForNeighbor_;
};
} // namespace facebook::fboss
