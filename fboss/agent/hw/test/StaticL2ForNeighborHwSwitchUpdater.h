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

#include "fboss/agent/StaticL2ForNeighborUpdater.h"

namespace facebook::fboss {
class SwitchState;
class HwSwitchEnsemble;

class StaticL2ForNeighborHwSwitchUpdater : public StaticL2ForNeighborUpdater {
 public:
  explicit StaticL2ForNeighborHwSwitchUpdater(
      HwSwitchEnsemble* hwSwitchEnsemble);

 private:
  template <typename NeighborEntryT>
  void ensureMacEntry(
      VlanID vlan,
      const std::shared_ptr<NeighborEntryT>& neighbor);

  void ensureMacEntryForNeighbor(
      VlanID vlan,
      const std::shared_ptr<ArpEntry>& neighbor) override;
  void ensureMacEntryForNeighbor(
      VlanID vlan,
      const std::shared_ptr<NdpEntry>& neighbor) override;

  template <typename NeighborEntryT>
  void pruneMacEntry(
      VlanID vlan,
      const std::shared_ptr<NeighborEntryT>& removedNeighbor);

  void pruneMacEntryForNeighbor(
      VlanID vlan,
      const std::shared_ptr<ArpEntry>& neighbor) override;
  void pruneMacEntryForNeighbor(
      VlanID vlan,
      const std::shared_ptr<NdpEntry>& neighbor) override;

  void ensureMacEntryIfNeighborExists(
      VlanID vlan,
      const std::shared_ptr<MacEntry>& macEntry) override;

  HwSwitchEnsemble* hwSwitchEnsemble_;
};
} // namespace facebook::fboss
