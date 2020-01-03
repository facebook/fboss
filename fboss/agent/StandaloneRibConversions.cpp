/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/StandaloneRibConversions.h"

#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"

namespace facebook::fboss {

rib::RoutingInformationBase switchStateToStandaloneRib(
    const std::shared_ptr<RouteTableMap>& swStateRib) {
  auto serializedSwState = swStateRib->toFollyDynamic();
  folly::dynamic serialized = folly::dynamic::object;
  for (const auto& entry : serializedSwState[kEntries]) {
    serialized[folly::to<std::string>(entry[kRouterId].asInt())] = entry;
  }
  return rib::RoutingInformationBase::fromFollyDynamic(serialized);
}

std::shared_ptr<RouteTableMap> standaloneToSwitchStateRib(
    const rib::RoutingInformationBase& standaloneRib) {
  auto serializedRib = standaloneRib.toFollyDynamic();
  folly::dynamic serialized = folly::dynamic::object;
  serialized[kExtraFields] = folly::dynamic::object;
  serialized[kEntries] = folly::dynamic::array;
  for (const auto& entry : serializedRib.values()) {
    serialized[kEntries].push_back(entry);
  }
  return RouteTableMap::fromFollyDynamic(serialized);
}

static void dynamicFibUpdate(
    facebook::fboss::RouterID vrf,
    const facebook::fboss::rib::IPv4NetworkToRouteMap& v4NetworkToRoute,
    const facebook::fboss::rib::IPv6NetworkToRouteMap& v6NetworkToRoute,
    void* cookie) {
  rib::ForwardingInformationBaseUpdater fibUpdater(
      vrf, v4NetworkToRoute, v6NetworkToRoute);

  auto sw = static_cast<facebook::fboss::SwSwitch*>(cookie);
  sw->updateStateBlocking("", std::move(fibUpdater));
}

void syncFibWithStandaloneRib(
    rib::RoutingInformationBase& standaloneRib,
    SwSwitch* swSwitch) {
  for (auto routerID : standaloneRib.getVrfList()) {
    standaloneRib.update(
        routerID,
        ClientID(-1),
        AdminDistance(-1),
        {},
        {},
        false,
        "post-warmboot FIB sync",
        &dynamicFibUpdate,
        static_cast<void*>(swSwitch));
  }
}

} // namespace facebook::fboss
