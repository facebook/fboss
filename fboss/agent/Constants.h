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

namespace facebook {
namespace fboss {

auto constexpr kClassID = "classID";
auto constexpr kEcmpEgress = "ecmpEgress";
auto constexpr kEcmpEgressId = "ecmpEgressId";
auto constexpr kEcmpHosts = "ecmpHosts";
auto constexpr kEgress = "egress";
auto constexpr kEgressId = "egressId";
auto constexpr kFlags = "flags";
auto constexpr kFwdInfo = "forwardingInfo";
auto constexpr kHostTable = "hostTable";
auto constexpr kHosts = "hosts";
auto constexpr kHwSwitch = "hwSwitch";
auto constexpr kIntf = "intf";
auto constexpr kIntfId = "intfId";
auto constexpr kIntfTable = "intfTable";
auto constexpr kIp = "ip";
auto constexpr kLabel = "label";
auto constexpr kMac = "mac";
auto constexpr kMplsNextHops = "mplsNextHops";
auto constexpr kMplsTunnel = "mplsTunnel";
auto constexpr kNextHops = "nexthops";
auto constexpr kNextHopsMulti = "rib";
auto constexpr kPaths = "paths";
auto constexpr kPort = "port";
auto constexpr kPrefix = "prefix";
auto constexpr kStack = "stack";
auto constexpr kSwSwitch = "swSwitch";
auto constexpr kVlan = "vlan";
auto constexpr kVrf = "vrf";
auto constexpr kWarmBootCache = "warmBootCache";

inline folly::StringPiece constexpr kWeight() {
  return "weight";
}

} // namespace fboss
} // namespace facebook
