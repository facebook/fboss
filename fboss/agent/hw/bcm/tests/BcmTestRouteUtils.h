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

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {
class RouteTableMap;

} // namespace facebook::fboss

namespace facebook::fboss::utility {

// Default RouterID
const auto kRouter0 = RouterID(0);

constexpr AdminDistance DISTANCE = AdminDistance::MAX_ADMIN_DISTANCE;

std::shared_ptr<RouteTableMap> addRoute(
    std::shared_ptr<RouteTableMap> routeTables,
    folly::CIDRNetwork network,
    folly::IPAddress nexthop);

std::shared_ptr<RouteTableMap> addRoute(
    std::shared_ptr<RouteTableMap> routeTables,
    folly::CIDRNetwork network,
    const std::vector<folly::IPAddress>& nexthopAddrs);

std::shared_ptr<RouteTableMap> deleteRoute(
    std::shared_ptr<RouteTableMap> routeTables,
    folly::CIDRNetwork network);

} // namespace facebook::fboss::utility
