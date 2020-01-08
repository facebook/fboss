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
#include "fboss/agent/rib/Route.h"
#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

class SwitchState;

} // namespace facebook::fboss

namespace facebook::fboss::rib {

class RouteNextHopEntry;

class ForwardingInformationBaseUpdater {
 public:
  ForwardingInformationBaseUpdater(
      RouterID vrf,
      const IPv4NetworkToRouteMap& v4NetworkToRoute,
      const IPv6NetworkToRouteMap& v6NetworkToRoute);

  std::shared_ptr<SwitchState> operator()(
      const std::shared_ptr<SwitchState>& state);

  static facebook::fboss::RouteNextHopEntry toFibNextHop(
      const RouteNextHopEntry& ribNextHopEntry);
  template <typename AddrT>
  static std::unique_ptr<facebook::fboss::Route<AddrT>> toFibRoute(
      const Route<AddrT>& ribRoute);

 private:
  template <typename AddressT>
  std::unique_ptr<typename facebook::fboss::ForwardingInformationBase<AddressT>>
  createUpdatedFib(
      const facebook::fboss::rib::NetworkToRouteMap<AddressT>& rib,
      const std::shared_ptr<
          facebook::fboss::ForwardingInformationBase<AddressT>>& fib);

  RouterID vrf_;
  const IPv4NetworkToRouteMap& v4NetworkToRoute_;
  const IPv6NetworkToRouteMap& v6NetworkToRoute_;
};

} // namespace facebook::fboss::rib
