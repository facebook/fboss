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

#include "fboss/agent/StateObserver.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class SwitchState;
class StateDelta;
class StaticL2ForNeighborObserver : public AutoRegisterStateObserver {
 public:
  explicit StaticL2ForNeighborObserver(SwSwitch* sw)
      : AutoRegisterStateObserver(sw, "StaticL2ForNeighborObserver"), sw_(sw) {}
  ~StaticL2ForNeighborObserver() override {}

  void stateUpdated(const StateDelta& stateDelta) override;

 private:
  template <typename AddrT>
  auto getTableDelta(const VlanDelta& vlanDelta);

  template <typename AddrT>
  void processUpdates(const StateDelta& stateDelta);

  template <typename AddedEntryT>
  void processAdded(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<AddedEntryT>& addedEntry);
  template <typename RemovedEntryT>
  void processRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      VlanID vlan,
      const std::shared_ptr<RemovedEntryT>& removedEntry);
  template <typename ChangedEntryT>
  void processChanged(
      const StateDelta& stateDelta,
      VlanID vlan,
      const std::shared_ptr<ChangedEntryT>& oldEntry,
      const std::shared_ptr<ChangedEntryT>& newEntry);

  SwSwitch* sw_;
};
} // namespace facebook::fboss
