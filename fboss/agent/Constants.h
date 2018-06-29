/*
 *  copyright (c) 2004-present, facebook, inc.
 *  all rights reserved.
 *
 *  this source code is licensed under the bsd-style license found in the
 *  license file in the root directory of this source tree. an additional grant
 *  of patent rights can be found in the patents file in the same directory.
 *
 */

#pragma once

#include <folly/String.h>

namespace facebook { namespace fboss {

auto constexpr kCpuPortName = "cpu";
auto constexpr kEcmpEgressId = "ecmpEgressId";
auto constexpr kEcmpEgress = "ecmpEgress";
auto constexpr kEcmpHosts = "ecmpHosts";
auto constexpr kEgress = "egress";
auto constexpr kEgressId = "egressId";
auto constexpr kHosts = "hosts";
auto constexpr kHostTable = "hostTable";
auto constexpr kHwSwitch = "hwSwitch";
auto constexpr kIntfId = "intfId";
auto constexpr kIntfTable = "intfTable";
auto constexpr kIp = "ip";
auto constexpr kMac = "mac";
auto constexpr kPaths = "paths";
auto constexpr kRouteTable = "routeTable";
auto constexpr kSwSwitch = "swSwitch";
auto constexpr kVrf = "vrf";
auto constexpr kWarmBootCache = "warmBootCache";

inline folly::StringPiece constexpr kWeight() {
  return "weight";
}

}}
