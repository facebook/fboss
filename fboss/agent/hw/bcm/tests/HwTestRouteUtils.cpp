/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestRouteUtils.h"

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

extern "C" {
#include <bcm/l3.h>
}

namespace facebook::fboss::utility {

std::optional<cfg::AclLookupClass> getHwRouteClassID(
    const HwSwitch* hwSwitch,
    RouterID /*rid*/,
    const folly::CIDRNetwork& cidrNetwork) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);
  const auto& [networkIP, netmask] = cidrNetwork;

  bcm_l3_route_t route;
  bcm_l3_route_t_init(&route);
  if (networkIP.isV4()) {
    route.l3a_subnet = networkIP.asV4().toLongHBO();
    route.l3a_ip_mask =
        folly::IPAddressV4(folly::IPAddressV4::fetchMask(netmask)).toLongHBO();
  } else { // IPv6
    ipToBcmIp6(networkIP, &route.l3a_ip6_net);
    memcpy(
        &route.l3a_ip6_mask,
        folly::IPAddressV6::fetchMask(netmask).data(),
        sizeof(route.l3a_ip6_mask));
    route.l3a_flags = BCM_L3_IP6;
  }

  CHECK_EQ(bcm_l3_route_get(bcmSwitch->getUnit(), &route), 0);

  return route.l3a_lookup_class == 0
      ? std::nullopt
      : std::optional(cfg::AclLookupClass{route.l3a_lookup_class});
}

} // namespace facebook::fboss::utility
