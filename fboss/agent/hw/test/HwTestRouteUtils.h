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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include "fboss/agent/HwSwitch.h"

#include <folly/IPAddress.h>

namespace facebook::fboss::utility {

std::optional<cfg::AclLookupClass> getHwRouteClassID(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork);

bool isHwRouteToCpu(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork);

bool isHwRouteMultiPath(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork);

bool isHwRouteToNextHop(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork,
    folly::IPAddress ip,
    std::optional<uint64_t> weight = std::nullopt);

bool isHwRouteHit(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork);

void clearHwRouteHit(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork);

uint64_t getRouteStat(
    const HwSwitch* hwSwitch,
    std::optional<RouteCounterID> counterID);

bool isHwRoutePresent(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork);

bool isRouteCounterSupported(const HwSwitch* hwSwitch);

} // namespace facebook::fboss::utility
