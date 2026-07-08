/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/rib/VrfRouteTable.h"

namespace facebook::fboss {

state::RouteTableFields VrfRouteTable::toThrift() const {
  state::RouteTableFields obj{};
  obj.v4NetworkToRoute() = v4NetworkToRoute.toThrift();
  obj.v6NetworkToRoute() = v6NetworkToRoute.toThrift();
  obj.labelToRoute() = labelToRoute.toThrift();
  return obj;
}

state::RouteTableFields VrfRouteTable::warmBootState() const {
  state::RouteTableFields obj{};
  obj.v4NetworkToRoute() = v4NetworkToRoute.warmBootState();
  obj.v6NetworkToRoute() = v6NetworkToRoute.warmBootState();
  obj.labelToRoute() = labelToRoute.warmBootState();
  return obj;
}

VrfRouteTable VrfRouteTable::fromThrift(const state::RouteTableFields& obj) {
  VrfRouteTable routeTable;
  routeTable.v4NetworkToRoute =
      IPv4NetworkToRouteMap::fromThrift(*obj.v4NetworkToRoute());
  routeTable.v6NetworkToRoute =
      IPv6NetworkToRouteMap::fromThrift(*obj.v6NetworkToRoute());
  routeTable.labelToRoute = LabelToRouteMap::fromThrift(*obj.labelToRoute());
  return routeTable;
}

} // namespace facebook::fboss
