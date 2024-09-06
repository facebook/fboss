/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestNeighborUtils.h"

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

extern "C" {
#include <bcm/l3.h>
}

namespace facebook::fboss::utility {
namespace {

bcm_l3_host_t getHost(int unit, const folly::IPAddress& ip, uint32_t flags) {
  bcm_l3_host_t host;
  bcm_l3_host_t_init(&host);
  if (ip.isV4()) {
    host.l3a_ip_addr = ip.asV4().toLongHBO();
  } else {
    host.l3a_flags |= BCM_L3_IP6;
    ipToBcmIp6(ip.asV6(), &(host.l3a_ip6_addr));
  }
  host.l3a_flags |= flags;
  auto rv = bcm_l3_host_find(unit, &host);
  bcmCheckError(rv, "Unable to find host: ", ip);
  return host;
}

bcm_l3_route_t getRoute(int unit, const folly::IPAddress& ip, uint32_t flags) {
  bcm_l3_route_t route;
  bcm_l3_route_t_init(&route);
  if (ip.isV4()) {
    route.l3a_subnet = ip.asV4().toLongHBO();
    route.l3a_ip_mask =
        folly::IPAddressV4(folly::IPAddressV4::fetchMask(32)).toLongHBO();
  } else {
    memcpy(
        &route.l3a_ip6_net,
        ip.asV6().toByteArray().data(),
        sizeof(route.l3a_ip6_net));
    memcpy(
        &route.l3a_ip6_mask,
        folly::IPAddressV6::fetchMask(128).data(),
        sizeof(route.l3a_ip6_mask));
    route.l3a_flags |= BCM_L3_IP6;
  }
  route.l3a_flags |= flags;
  auto rv = bcm_l3_route_get(unit, &route);
  bcmCheckError(rv, "Unable to find route: ", ip);
  return route;
}
} // namespace

bool isHostHit(const HwSwitch* hwSwitch, const folly::IPAddress& ip) {
  if (hwSwitch->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::HOSTTABLE)) {
    auto host =
        getHost(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip, 0);
    return host.l3a_flags & BCM_L3_HIT;
  }
  auto route =
      getRoute(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip, 0);
  return route.l3a_flags & BCM_L3_HIT;
}

void clearHostHitBit(const HwSwitch* hwSwitch, const folly::IPAddress& ip) {
  if (hwSwitch->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::HOSTTABLE)) {
    getHost(
        static_cast<const BcmSwitch*>(hwSwitch)->getUnit(),
        ip,
        BCM_L3_HIT_CLEAR);
  } else {
    getRoute(
        static_cast<const BcmSwitch*>(hwSwitch)->getUnit(),
        ip,
        BCM_L3_HIT_CLEAR);
  }
}

} // namespace facebook::fboss::utility
