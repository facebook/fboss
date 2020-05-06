/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/LookupClassRouteUpdater.h"

namespace facebook::fboss {

// Methods for handling port updates
void LookupClassRouteUpdater::processPortAdded(
    const StateDelta& /*stateDelta */,
    const std::shared_ptr<Port>& /*addedPort*/) {}

void LookupClassRouteUpdater::processPortRemoved(
    const StateDelta& /*stateDelta */,
    const std::shared_ptr<Port>& /*port*/) {}

void LookupClassRouteUpdater::processPortChanged(
    const StateDelta& /*stateDelta*/,
    const std::shared_ptr<Port>& /*oldPort*/,
    const std::shared_ptr<Port>& /*newPort*/) {}

void LookupClassRouteUpdater::processPortUpdates(const StateDelta& stateDelta) {
  for (const auto& delta : stateDelta.getPortsDelta()) {
    auto oldPort = delta.getOld();
    auto newPort = delta.getNew();

    if (!oldPort && newPort) {
      processPortAdded(stateDelta, newPort);
    } else if (oldPort && !newPort) {
      processPortRemoved(stateDelta, oldPort);
    } else {
      processPortChanged(stateDelta, oldPort, newPort);
    }
  }
}

// Methods for handling neighbor updates
template <typename AddedNeighborT>
void LookupClassRouteUpdater::processNeighborAdded(
    const StateDelta& /*stateDelta*/,
    VlanID /*vlan*/,
    const std::shared_ptr<AddedNeighborT>& /*addedNeighbor*/) {}

template <typename RemovedNeighborT>
void LookupClassRouteUpdater::processNeighborRemoved(
    const StateDelta& /*stateDelta*/,
    VlanID /*vlan*/,
    const std::shared_ptr<RemovedNeighborT>& /*removedNeighbor*/) {}

template <typename ChangedNeighborT>
void LookupClassRouteUpdater::processNeighborChanged(
    const StateDelta& /*stateDelta*/,
    VlanID /*vlan*/,
    const std::shared_ptr<ChangedNeighborT>& /*oldNeighbor*/,
    const std::shared_ptr<ChangedNeighborT>& /*newNeighbor*/) {}

template <typename AddrT>
void LookupClassRouteUpdater::processNeighborUpdates(
    const StateDelta& stateDelta) {
  {
    using NeighborTableT = std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    for (const auto& vlanDelta : stateDelta.getVlansDelta()) {
      auto newVlan = vlanDelta.getNew();
      if (!newVlan) {
        auto oldVlan = vlanDelta.getOld();
        for (const auto& oldNeighbor :
             *(oldVlan->template getNeighborTable<NeighborTableT>())) {
          processNeighborRemoved(stateDelta, oldVlan->getID(), oldNeighbor);
        }
        continue;
      }

      auto vlan = newVlan->getID();

      for (const auto& delta :
           vlanDelta.template getNeighborDelta<NeighborTableT>()) {
        auto oldNeighbor = delta.getOld();
        auto newNeighbor = delta.getNew();

        /*
         * At this point in time, queue-per-host fix is needed (and thus
         * supported) for physical link only.
         */
        if ((oldNeighbor && !oldNeighbor->getPort().isPhysicalPort()) ||
            (newNeighbor && !newNeighbor->getPort().isPhysicalPort())) {
          continue;
        }

        if (!oldNeighbor) {
          processNeighborAdded(stateDelta, vlan, newNeighbor);
        } else if (!newNeighbor) {
          processNeighborRemoved(stateDelta, vlan, oldNeighbor);
        } else {
          processNeighborChanged(stateDelta, vlan, oldNeighbor, newNeighbor);
        }
      }
    }
  }
}

// Methods for handling route updates
template <typename RouteT>
void LookupClassRouteUpdater::processRouteAdded(
    const StateDelta& /*stateDelta*/,
    RouterID /*rid*/,
    const std::shared_ptr<RouteT>& /*addedRoute*/) {}

template <typename RouteT>
void LookupClassRouteUpdater::processRouteRemoved(
    const StateDelta& /*stateDelta*/,
    RouterID /*rid*/,
    const std::shared_ptr<RouteT>& /*removedRoute*/) {}

template <typename RouteT>
void LookupClassRouteUpdater::processRouteChanged(
    const StateDelta& /*stateDelta*/,
    RouterID /*rid*/,
    const std::shared_ptr<RouteT>& /*oldRoute*/,
    const std::shared_ptr<RouteT>& /*newRoute*/) {}

template <typename AddrT>
void LookupClassRouteUpdater::processRouteUpdates(
    const StateDelta& stateDelta) {
  for (auto const& routeTableDelta : stateDelta.getRouteTablesDelta()) {
    auto const& newRouteTable = routeTableDelta.getNew();
    if (!newRouteTable) {
      auto const& oldRouteTable = routeTableDelta.getOld();
      for (const auto& oldRoute :
           *(oldRouteTable->template getRib<AddrT>()->routes())) {
        processRouteRemoved(stateDelta, oldRouteTable->getID(), oldRoute);
      }
      continue;
    }

    auto rid = newRouteTable->getID();
    for (auto const& routeDelta : routeTableDelta.getRoutesDelta<AddrT>()) {
      auto const& oldRoute = routeDelta.getOld();
      auto const& newRoute = routeDelta.getNew();

      if (!oldRoute) {
        processRouteAdded(stateDelta, rid, newRoute);
      } else if (!newRoute) {
        processRouteRemoved(stateDelta, rid, oldRoute);
      } else {
        processRouteChanged(stateDelta, rid, oldRoute, newRoute);
      }
    }
  }
}

void LookupClassRouteUpdater::stateUpdated(const StateDelta& stateDelta) {
  processPortUpdates(stateDelta);

  processNeighborUpdates<folly::IPAddressV6>(stateDelta);
  processNeighborUpdates<folly::IPAddressV4>(stateDelta);

  processRouteUpdates<folly::IPAddressV6>(stateDelta);
  processRouteUpdates<folly::IPAddressV4>(stateDelta);
}

} // namespace facebook::fboss
