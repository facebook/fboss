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
#include "fboss/agent/hw/bcm/BcmRouteCounter.h"
#include "fboss/agent/hw/bcm/BcmSdkVer.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

extern "C" {
#include <bcm/flexctr.h>
#include <bcm/l3.h>
}

namespace facebook::fboss::utility {

bcm_l3_route_t
getBcmRoute(int unit, const folly::CIDRNetwork& cidrNetwork, uint32_t flags) {
  bcm_l3_route_t route;
  bcm_l3_route_t_init(&route);

  const auto& [networkIP, netmask] = cidrNetwork;
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
  route.l3a_flags |= flags;
  CHECK_EQ(bcm_l3_route_get(unit, &route), 0);
  return route;
}

bool isRoutePresent(
    int unit,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork) {
  bcm_l3_route_t route;
  bcm_l3_route_t_init(&route);

  route.l3a_vrf = rid;
  const auto& [networkIP, netmask] = cidrNetwork;
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
  return (0 == bcm_l3_route_get(unit, &route));
}

uint64_t getBcmRouteCounter(
    const HwSwitch* hwSwitch,
    std::optional<RouteCounterID> id) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);
  uint32 entry = 0;
  bcm_stat_value_t routeCounter;
  auto unit = bcmSwitch->getUnit();
  CHECK(bcmSwitch->routeCounterTable()->getHwCounterID(id).has_value());
  auto hwCounterId = *bcmSwitch->routeCounterTable()->getHwCounterID(id);
  auto counterIndex = hwCounterId.getHwId();
  if (bcmSwitch->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::ROUTE_FLEX_COUNTERS)) {
#if defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_20)
    uint32 counterOffset = hwCounterId.getHwOffset();
    bcm_flexctr_counter_value_t counterValue;
    CHECK_EQ(
        bcm_flexctr_stat_sync_get(
            unit, counterIndex, 1, &counterOffset, &counterValue),
        0);
    XLOG(DBG2) << "Route counter id: " << hwCounterId.str()
               << " offset: " << counterIndex
               << " bytes: " << COMPILER_64_LO(counterValue.value[1]);
    return COMPILER_64_LO(counterValue.value[1]);
#else
    return 0;
#endif
  } else {
    CHECK_EQ(
        bcm_stat_flex_counter_sync_get(
            unit, counterIndex, bcmStatFlexStatBytes, 1, &entry, &routeCounter),

        0);
    XLOG(DBG2) << "Route counter id: " << hwCounterId.str()
               << " bytes: " << COMPILER_64_LO(routeCounter.bytes);
    return COMPILER_64_LO(routeCounter.bytes);
  }
}

bool isEgressToIp(
    const BcmSwitch* bcmSwitch,
    RouterID rid,
    folly::IPAddress addr,
    bcm_if_t egress) {
  if (!bcmSwitch->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::HOSTTABLE)) {
    bcm_l3_route_t route = getBcmRoute(
        bcmSwitch->getUnit(), folly::CIDRNetwork(addr, addr.bitCount()), 0);
    return egress == route.l3a_intf;
  }
  bcm_l3_host_t host;
  bcm_l3_host_t_init(&host);
  if (addr.isV4()) {
    host.l3a_ip_addr = addr.asV4().toLongHBO();
  } else {
    memcpy(
        &host.l3a_ip6_addr,
        addr.asV6().toByteArray().data(),
        sizeof(host.l3a_ip6_addr));
    host.l3a_flags |= BCM_L3_IP6;
  }
  host.l3a_vrf = rid;
  CHECK_EQ(bcm_l3_host_find(bcmSwitch->getUnit(), &host), 0);
  return egress == host.l3a_intf;
}

std::optional<cfg::AclLookupClass> getHwRouteClassID(
    const HwSwitch* hwSwitch,
    RouterID /*rid*/,
    const folly::CIDRNetwork& cidrNetwork) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  bcm_l3_route_t route = getBcmRoute(bcmSwitch->getUnit(), cidrNetwork, 0);

  return route.l3a_lookup_class == 0
      ? std::nullopt
      : std::optional(cfg::AclLookupClass{route.l3a_lookup_class});
}

bool isHwRouteToCpu(
    const HwSwitch* hwSwitch,
    RouterID /*rid*/,
    const folly::CIDRNetwork& cidrNetwork) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  bcm_l3_route_t route = getBcmRoute(bcmSwitch->getUnit(), cidrNetwork, 0);

  if (route.l3a_flags & BCM_L3_MULTIPATH) {
    return false;
  }
  bcm_l3_egress_t egress;
  bcm_l3_egress_t_init(&egress);
  CHECK_EQ(bcm_l3_egress_get(bcmSwitch->getUnit(), route.l3a_intf, &egress), 0);

  return (egress.flags & BCM_L3_L2TOCPU) && (egress.flags | BCM_L3_COPY_TO_CPU);
}

bool isHwRouteHit(
    const HwSwitch* hwSwitch,
    RouterID /*rid*/,
    const folly::CIDRNetwork& cidrNetwork) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);
  bcm_l3_route_t route = getBcmRoute(bcmSwitch->getUnit(), cidrNetwork, 0);
  return (route.l3a_flags & BCM_L3_HIT);
}

void clearHwRouteHit(
    const HwSwitch* hwSwitch,
    RouterID /*rid*/,
    const folly::CIDRNetwork& cidrNetwork) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);
  getBcmRoute(bcmSwitch->getUnit(), cidrNetwork, BCM_L3_HIT_CLEAR);
}

bool isHwRouteMultiPath(
    const HwSwitch* hwSwitch,
    RouterID /*rid*/,
    const folly::CIDRNetwork& cidrNetwork) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  bcm_l3_route_t route = getBcmRoute(bcmSwitch->getUnit(), cidrNetwork, 0);

  return route.l3a_flags & BCM_L3_MULTIPATH;
}

bool isHwRouteToNextHop(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork,
    folly::IPAddress ip,
    std::optional<uint64_t> weight) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  bcm_l3_route_t route = getBcmRoute(bcmSwitch->getUnit(), cidrNetwork, 0);

  if (route.l3a_flags & BCM_L3_MULTIPATH) {
    // check for member to ip, interface
    bcm_l3_egress_ecmp_t ecmp;
    bcm_l3_egress_ecmp_t_init(&ecmp);
    ecmp.ecmp_intf = route.l3a_intf;
    ecmp.flags = BCM_L3_WITH_ID;
    int count = 0;
    CHECK_EQ(
        bcm_l3_ecmp_get(bcmSwitch->getUnit(), &ecmp, 0, nullptr, &count), 0);
    std::vector<bcm_l3_ecmp_member_t> members;
    members.resize(count);
    CHECK_EQ(
        bcm_l3_ecmp_get(
            bcmSwitch->getUnit(),
            &ecmp,
            members.size(),
            members.data(),
            &count),
        0);
    bool found = false;
    bcm_l3_ecmp_member_t foundMember;
    bcm_l3_ecmp_member_t_init(&foundMember);
    for (auto member : members) {
      if (!isEgressToIp(bcmSwitch, rid, ip, member.egress_if)) {
        continue;
      }
      found = true;
      foundMember = member;
      break;
    }

    if (!weight) {
      return found;
    }

    return weight.value() ==
        std::count_if(
               members.begin(), members.end(), [foundMember](auto member) {
                 return member.egress_if == foundMember.egress_if;
               });
  }
  // check for next hop
  return isEgressToIp(bcmSwitch, rid, ip, route.l3a_intf);
}

uint64_t getRouteStat(
    const HwSwitch* hwSwitch,
    std::optional<RouteCounterID> counterID) {
  return getBcmRouteCounter(hwSwitch, counterID);
}

bool isHwRoutePresent(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);
  return isRoutePresent(bcmSwitch->getUnit(), rid, cidrNetwork);
}

} // namespace facebook::fboss::utility
