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
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

class LookupClassRouteUpdater : public AutoRegisterStateObserver {
 public:
  explicit LookupClassRouteUpdater(SwSwitch* sw)
      : AutoRegisterStateObserver(sw, "LookupClassRouteUpdater") {}
  ~LookupClassRouteUpdater() override {}

  void stateUpdated(const StateDelta& stateDelta) override;

 private:
  // Methods for handling port updates
  void processPortAdded(
      const StateDelta& stateDelta,
      const std::shared_ptr<Port>& addedPort);
  void processPortRemoved(
      const StateDelta& stateDelta,
      const std::shared_ptr<Port>& port);
  void processPortChanged(
      const StateDelta& stateDelta,
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  void processPortUpdates(const StateDelta& stateDelta);

  // Methods for handling neighbor updates
  template <typename AddedNeighborT>
  void processNeighborAdded(
      const StateDelta& stateDelta,
      VlanID vlan,
      const std::shared_ptr<AddedNeighborT>& addedNeighbor);
  template <typename removedNeighborT>
  void processNeighborRemoved(
      const StateDelta& stateDelta,
      VlanID vlan,
      const std::shared_ptr<removedNeighborT>& removedNeighbor);
  template <typename ChangedNeighborT>
  void processNeighborChanged(
      const StateDelta& stateDelta,
      VlanID vlan,
      const std::shared_ptr<ChangedNeighborT>& oldNeighbor,
      const std::shared_ptr<ChangedNeighborT>& newNeighbor);

  template <typename AddrT>
  void processNeighborUpdates(const StateDelta& stateDelta);

  // Methods for handling route updates
  template <typename RouteT>
  void processRouteAdded(
      const StateDelta& stateDelta,
      RouterID rid,
      const std::shared_ptr<RouteT>& addedRoute);
  template <typename RouteT>
  void processRouteRemoved(
      const StateDelta& stateDelta,
      RouterID rid,
      const std::shared_ptr<RouteT>& removedRoute);
  template <typename RouteT>
  void processRouteChanged(
      const StateDelta& stateDelta,
      RouterID rid,
      const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute);

  template <typename AddrT>
  void processRouteUpdates(const StateDelta& stateDelta);
};

} // namespace facebook::fboss
