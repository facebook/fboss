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

#include "fboss/agent/rib/NetworkToRouteMap.h"
#include "fboss/agent/rib/NextHopIDManager.h"

#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

class SwitchState;
class SwitchIdScopeResolver;

class ForwardingInformationBaseUpdater {
 public:
  /*
   * Constructor with NextHopIDManager for clients that need ECMP NextHop ID
   * allocation.
   *
   * The nextHopIDManager parameter is passed as const pointer
   * even though this class internally modifies its state.
   *
   * In the long-term design, NextHop ID allocation
   * will happen directly in the RIB layer, not in the FIB updater.
   * The const interface reflects this intended ownership model where
   * the FIB updater should ideally only read from the manager.
   *
   * During this temporary phase, we use const_cast
   * internally to perform ID allocation/deallocation. This approach allows
   * us to maintain a clean const interface while still enabling the
   * temporary functionality needed before the RIB-based allocation is
   * complete.
   */
  ForwardingInformationBaseUpdater(
      const SwitchIdScopeResolver* resolver,
      RouterID vrf,
      const IPv4NetworkToRouteMap& v4NetworkToRoute,
      const IPv6NetworkToRouteMap& v6NetworkToRoute,
      const LabelToRouteMap& labelToRoute,
      const NextHopIDManager* nextHopIDManager);

  ForwardingInformationBaseUpdater(
      const SwitchIdScopeResolver* resolver,
      RouterID vrf,
      const IPv4NetworkToRouteMap& v4NetworkToRoute,
      const IPv6NetworkToRouteMap& v6NetworkToRoute,
      const LabelToRouteMap& labelToRoute)
      : ForwardingInformationBaseUpdater(
            resolver,
            vrf,
            v4NetworkToRoute,
            v6NetworkToRoute,
            labelToRoute,
            nullptr) {}

  std::shared_ptr<SwitchState> operator()(
      const std::shared_ptr<SwitchState>& state);
  std::optional<StateDelta> getLastDelta() const {
    return lastDelta_
        ? StateDelta(lastDelta_->oldState(), lastDelta_->newState())
        : std::optional<StateDelta>();
  }

 private:
  /*
   * Return updated FIB on change, null otherwise
   */
  template <typename AddressT>
  std::shared_ptr<typename facebook::fboss::ForwardingInformationBase<AddressT>>
  createUpdatedFib(
      const facebook::fboss::NetworkToRouteMap<AddressT>& rib,
      const std::shared_ptr<
          facebook::fboss::ForwardingInformationBase<AddressT>>& fib,
      std::shared_ptr<SwitchState>& state,
      NextHopIDManager* nextHopIDManager);
  std::shared_ptr<facebook::fboss::MultiLabelForwardingInformationBase>
  createUpdatedLabelFib(
      const facebook::fboss::NetworkToRouteMap<LabelID>& rib,
      std::shared_ptr<facebook::fboss::MultiLabelForwardingInformationBase>
          fib);

  const SwitchIdScopeResolver* resolver_;
  RouterID vrf_;
  const IPv4NetworkToRouteMap& v4NetworkToRoute_;
  const IPv6NetworkToRouteMap& v6NetworkToRoute_;
  const LabelToRouteMap& labelToRoute_;
  const NextHopIDManager* nextHopIDManager_;
  std::optional<StateDelta> lastDelta_;
};

} // namespace facebook::fboss
