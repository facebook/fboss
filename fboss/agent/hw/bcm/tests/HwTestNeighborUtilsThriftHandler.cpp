// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/test/utils/AclTestUtils.h"

using facebook::fboss::bcmCheckError;
using facebook::fboss::BcmError;
using facebook::fboss::BcmSwitch;
using facebook::fboss::HwAsic;
using facebook::fboss::HwSwitch;
using facebook::fboss::InterfaceID;
using facebook::fboss::ipToBcmIp6;

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

bool nbrExists(
    const HwSwitch* hwSwitch,
    InterfaceID /*intf*/,
    const folly::IPAddress& ip) {
  try {
    if (hwSwitch->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::HOSTTABLE)) {
      getHost(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip, 0);
    } else {
      getRoute(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip, 0);
    }
  } catch (const BcmError& e) {
    if (e.getBcmError() == BCM_E_NOT_FOUND) {
      return false;
    } else {
      throw;
    }
  }
  return true;
}

bool nbrProgrammedToCpu(const HwSwitch* hwSwitch, const folly::IPAddress& ip) {
  bcm_if_t intf;
  try {
    if (hwSwitch->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::HOSTTABLE)) {
      auto host =
          getHost(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip, 0);
      intf = host.l3a_intf;
    } else {
      auto route =
          getRoute(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip, 0);
      intf = route.l3a_intf;
    }
  } catch (const BcmError& e) {
    if (e.getBcmError() == BCM_E_NOT_FOUND) {
      return false;
    } else {
      throw;
    }
  }
  bcm_l3_egress_t egress;
  auto cpuFlags = (BCM_L3_L2TOCPU | BCM_L3_COPY_TO_CPU);
  bcm_l3_egress_t_init(&egress);
  bcm_l3_egress_get(0, intf, &egress);
  return (egress.flags & cpuFlags) == cpuFlags;
}

std::optional<uint32_t> getNbrClassId(
    const HwSwitch* hwSwitch,
    const folly::IPAddress& ip) {
  try {
    if (hwSwitch->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::HOSTTABLE)) {
      auto host =
          getHost(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip, 0);
      return host.l3a_lookup_class;
    } else {
      auto route =
          getRoute(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip, 0);
      return route.l3a_lookup_class;
    }
  } catch (const BcmError& e) {
    if (e.getBcmError() == BCM_E_NOT_FOUND) {
      return std::nullopt;
    } else {
      throw;
    }
  }
}

} // namespace

namespace facebook {
namespace fboss {
namespace utility {

void HwTestThriftHandler::getNeighborInfo(
    NeighborInfo& neighborInfo,
    std::unique_ptr<::facebook::fboss::IfAndIP> neighbor) {
  auto classId =
      getNbrClassId(hwSwitch_, facebook::network::toIPAddress(*neighbor->ip()));
  if (classId.has_value()) {
    neighborInfo.classId() = *classId;
  }
  neighborInfo.isProgrammedToCpu() = nbrProgrammedToCpu(
      hwSwitch_, facebook::network::toIPAddress(*neighbor->ip()));
  neighborInfo.exists() = nbrExists(
      hwSwitch_,
      InterfaceID(*neighbor->interfaceID()),
      facebook::network::toIPAddress(*neighbor->ip()));
}

} // namespace utility
} // namespace fboss
} // namespace facebook
