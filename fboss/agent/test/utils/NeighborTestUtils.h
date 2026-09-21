// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/StaticL2ForNeighborSwSwitchUpdater.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss::utility {

class NeighborTestUtils {
 public:
  template <typename NeighborEntryT>
  static std::shared_ptr<SwitchState> updateMacEntryForUpdatedNbrEntry(
      std::shared_ptr<SwitchState> switchState,
      VlanID vlan,
      const std::shared_ptr<NeighborEntryT>& oldEntry,
      const std::shared_ptr<NeighborEntryT>& newEntry) {
    auto outputState = switchState->clone();
    if (newEntry->getMac() != oldEntry->getMac() ||
        newEntry->getPort() != oldEntry->getPort() ||
        newEntry->getState() != oldEntry->getState()) {
      if (oldEntry->isReachable()) {
        outputState = StaticL2ForNeighborSwSwitchUpdater::pruneMacEntry(
            outputState, vlan, oldEntry);
      }
      if (newEntry->isReachable()) {
        outputState = StaticL2ForNeighborSwSwitchUpdater::ensureMacEntry(
            outputState, vlan, newEntry);
      }
    }
    return outputState;
  }

  template <typename NeighborEntryT>
  static std::shared_ptr<SwitchState> addMacEntryForNewNbrEntry(
      std::shared_ptr<SwitchState> switchState,
      VlanID vlan,
      const std::shared_ptr<NeighborEntryT>& newEntry) {
    if (newEntry->isReachable()) {
      return StaticL2ForNeighborSwSwitchUpdater::ensureMacEntry(
          switchState, vlan, newEntry);
    }
    return switchState;
  }

  template <typename NeighborEntryT>
  static std::shared_ptr<SwitchState> pruneMacEntryForDelNbrEntry(
      std::shared_ptr<SwitchState> switchState,
      VlanID vlan,
      const std::shared_ptr<NeighborEntryT>& delEntry) {
    if (delEntry->isReachable()) {
      return StaticL2ForNeighborSwSwitchUpdater::pruneMacEntry(
          switchState, vlan, delEntry);
    }
    return switchState;
  }
};

} // namespace facebook::fboss::utility
