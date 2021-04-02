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

#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/rib/ForwardingInformationBaseUpdater.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/SwitchState.h"

DEFINE_bool(
    enable_standalone_rib,
    false,
    "Place the RIB under the control of the RoutingInformationBase object");

namespace facebook::fboss {

std::unique_ptr<RoutingInformationBase> switchStateToStandaloneRib(
    const std::shared_ptr<RouteTableMap>& swStateRib) {
  auto serializedSwState = swStateRib->toFollyDynamic();
  folly::dynamic serialized = folly::dynamic::object;
  for (const auto& entry : serializedSwState[kEntries]) {
    serialized[folly::to<std::string>(entry[kRouterId].asInt())] = entry;
  }
  return RoutingInformationBase::fromFollyDynamic(
      serialized,
      // Empty FIB, all information is in serialized legacy RIB
      std::make_shared<ForwardingInformationBaseMap>());
}

std::shared_ptr<RouteTableMap> standaloneToSwitchStateRib(
    const RoutingInformationBase& standaloneRib) {
  auto serializedRib = standaloneRib.toFollyDynamic();
  folly::dynamic serialized = folly::dynamic::object;
  serialized[kExtraFields] = folly::dynamic::object;
  serialized[kEntries] = folly::dynamic::array;
  for (const auto& entry : serializedRib.values()) {
    serialized[kEntries].push_back(entry);
  }
  return RouteTableMap::fromFollyDynamic(serialized);
}

void programRib(RoutingInformationBase& standaloneRib, SwSwitch* swSwitch) {
  for (auto routerID : standaloneRib.getVrfList()) {
    standaloneRib.update(
        routerID,
        ClientID(-1),
        AdminDistance(-1),
        {},
        {},
        false,
        "post-warmboot FIB sync",
        &swSwitchFibUpdate,
        static_cast<void*>(swSwitch));
  }
}

std::shared_ptr<ForwardingInformationBaseMap> fibFromStandaloneRib(
    RoutingInformationBase& rib) {
  auto state = std::make_shared<SwitchState>();
  auto fillInFib =
      [&state](
          facebook::fboss::RouterID vrf,
          const facebook::fboss::IPv4NetworkToRouteMap& v4NetworkToRoute,
          const facebook::fboss::IPv6NetworkToRouteMap& v6NetworkToRoute,
          void* cookie) {
        facebook::fboss::ForwardingInformationBaseUpdater fibUpdater(
            vrf, v4NetworkToRoute, v6NetworkToRoute);
        fibUpdater(state);
        return state;
      };

  for (auto routerID : rib.getVrfList()) {
    rib.update(
        routerID,
        ClientID(-1),
        AdminDistance(-1),
        {},
        {},
        false,
        "rib to fib",
        fillInFib,
        nullptr);
  }
  return state->getFibs();
}

void handleStandaloneRIBTransition(
    const folly::dynamic& switchStateJson,
    HwInitResult& ret) {
  std::unique_ptr<RoutingInformationBase> deserializedRIB;
  if (switchStateJson.find(kRib) != switchStateJson.items().end()) {
    deserializedRIB = RoutingInformationBase::fromFollyDynamic(
        switchStateJson[kRib], ret.switchState->getFibs());
  }
  ret.switchState = ret.switchState->clone();
  if (FLAGS_enable_standalone_rib) {
    if (deserializedRIB) {
      // Common case - deser rib was successful
      ret.rib = std::move(deserializedRIB);

    } else {
      // Transitioning from no standalone RIB to standalone RIB
      // 1. Reconstruct new RIB from old rib
      // 2. Build FIB from RIB
      ret.rib = switchStateToStandaloneRib(ret.switchState->getRouteTables());
      ret.switchState->resetForwardingInformationBases(
          fibFromStandaloneRib(*ret.rib));
    }
    // reset old rib route tables
    ret.switchState->resetRouteTables(std::make_shared<RouteTableMap>());

  } else {
    if (deserializedRIB) {
      // Transitioning from standalone RIB to no standalone RIB
      // Emergency rollback + canary on/off handling
      ret.switchState->resetRouteTables(
          standaloneToSwitchStateRib(*deserializedRIB));
      ret.switchState->resetForwardingInformationBases(
          std::make_shared<ForwardingInformationBaseMap>());
    }
    ret.rib = nullptr;
  }
  ret.switchState->publish();
}
} // namespace facebook::fboss
