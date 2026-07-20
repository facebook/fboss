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

#include <algorithm>
#include <cstdint>
#include <string>

namespace facebook::fboss {

// Finds the static route in `routes` matching (vrfId, prefix), returning an
// iterator to it or routes.end() if none matches. Works for any of the static
// route lists (null0/CPU/nexthop) since they all expose routerID() and
// prefix().
template <typename RouteList>
auto findStaticRoute(
    RouteList& routes,
    int32_t vrfId,
    const std::string& prefix) {
  return std::find_if(routes.begin(), routes.end(), [&](const auto& route) {
    return *route.routerID() == vrfId && *route.prefix() == prefix;
  });
}

// Erases the static route in `routes` matching (vrfId, prefix), if present.
// Returns true if a matching route was found and erased, false otherwise.
template <typename RouteList>
bool eraseStaticRoute(
    RouteList& routes,
    int32_t vrfId,
    const std::string& prefix) {
  auto it = findStaticRoute(routes, vrfId, prefix);
  if (it == routes.end()) {
    return false;
  }
  routes.erase(it);
  return true;
}

} // namespace facebook::fboss
