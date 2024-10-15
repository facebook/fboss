// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/types.h"

using facebook::fboss::HwSwitch;
using facebook::fboss::RouterID;
using facebook::fboss::SaiApiTable;
using facebook::fboss::SaiNextHopTraitsT;
using facebook::fboss::SaiRouteTraits;
using facebook::fboss::SaiSwitch;
using facebook::fboss::SaiSwitchTraits;
using facebook::fboss::cfg::AclLookupClass;

namespace {
bool isEgressToIp(folly::IPAddress addr, sai_object_id_t adapterKey) {
  auto ipAttr = SaiApiTable::getInstance()->nextHopApi().getAttribute(
      static_cast<facebook::fboss::NextHopSaiId>(adapterKey),
      SaiNextHopTraitsT<SAI_NEXT_HOP_TYPE_IP>::Attributes::Ip());
  return ipAttr == addr;
}

SaiRouteTraits::RouteEntry getSaiRouteAdapterKey(
    const SaiSwitch* saiSwitch,
    RouterID rid,
    const folly::CIDRNetwork& prefix) {
  auto virtualRouterHandle =
      saiSwitch->managerTable()->virtualRouterManager().getVirtualRouterHandle(
          rid);
  if (!virtualRouterHandle) {
    throw facebook::fboss::FbossError("No virtual router with id 0");
  }

  return SaiRouteTraits::RouteEntry(
      saiSwitch->getSaiSwitchId(),
      virtualRouterHandle->virtualRouter->adapterKey(),
      prefix);
}

std::optional<int> getHwRouteClassID(
    const HwSwitch* hw,
    RouterID rid,
    const folly::CIDRNetwork& prefix) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hw);

  try {
    auto r = getSaiRouteAdapterKey(saiSwitch, rid, prefix);
    auto metadata = SaiApiTable::getInstance()->routeApi().getAttribute(
        r, SaiRouteTraits::Attributes::Metadata());
    return metadata == 0 ? std::nullopt : std::optional(metadata);
  } catch (const std::exception& e) {
    return std::nullopt;
  }
}

bool isHwRouteToCpu(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch);

  auto routeAdapterKey = getSaiRouteAdapterKey(saiSwitch, rid, cidrNetwork);
  sai_object_id_t nhop = SaiApiTable::getInstance()->routeApi().getAttribute(
      routeAdapterKey, SaiRouteTraits::Attributes::NextHopId());

  facebook::fboss::SwitchSaiId switchId =
      saiSwitch->managerTable()->switchManager().getSwitchSaiId();
  sai_object_id_t cpuPortId{
      SaiApiTable::getInstance()->switchApi().getAttribute(
          switchId, SaiSwitchTraits::Attributes::CpuPort{})};

  return nhop == cpuPortId;
}

bool isHwRouteHit(
    [[maybe_unused]] const HwSwitch* hwSwitch,
    [[maybe_unused]] RouterID /*rid*/,
    [[maybe_unused]] const folly::CIDRNetwork& cidrNetwork) {
  throw facebook::fboss::FbossError("L3 entry hitbit is unsupported for SAI");
}

void clearHwRouteHit(
    [[maybe_unused]] const HwSwitch* hwSwitch,
    [[maybe_unused]] RouterID /*rid*/,
    [[maybe_unused]] const folly::CIDRNetwork& cidrNetwork) {
  throw facebook::fboss::FbossError("L3 entry hitbit is unsupported for SAI");
}

bool isHwRouteMultiPath(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch);

  auto routeAdapterKey = getSaiRouteAdapterKey(saiSwitch, rid, cidrNetwork);
  sai_object_id_t nhop = SaiApiTable::getInstance()->routeApi().getAttribute(
      routeAdapterKey, SaiRouteTraits::Attributes::NextHopId());

  try {
    SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
        static_cast<facebook::fboss::NextHopGroupSaiId>(nhop),
        facebook::fboss::SaiNextHopGroupTraits::Attributes::
            NextHopMemberList{});
  } catch (const facebook::fboss::SaiApiError&) {
    // its not next hop group
    return false;
  }
  return true;
}

bool isHwRouteToNextHop(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork,
    folly::IPAddress ip,
    std::optional<uint64_t> weight) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch);
  auto classId = getHwRouteClassID(hwSwitch, rid, cidrNetwork);
  if (classId.has_value() &&
      (AclLookupClass(classId.value()) ==
           AclLookupClass::DST_CLASS_L3_LOCAL_1 ||
       AclLookupClass(classId.value()) ==
           AclLookupClass::DST_CLASS_L3_LOCAL_2)) {
    XLOG(DBG2) << "resolved route has class ID 1 or 2";
    return false;
  }

  auto routeAdapterKey = getSaiRouteAdapterKey(saiSwitch, rid, cidrNetwork);
  sai_object_id_t nhop = SaiApiTable::getInstance()->routeApi().getAttribute(
      routeAdapterKey, SaiRouteTraits::Attributes::NextHopId());

  try {
    auto members = SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
        static_cast<facebook::fboss::NextHopGroupSaiId>(nhop),
        facebook::fboss::SaiNextHopGroupTraits::Attributes::
            NextHopMemberList{});
    for (auto member : members) {
      sai_object_id_t nextHopId =
          SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
              static_cast<facebook::fboss::NextHopGroupMemberSaiId>(member),
              facebook::fboss::SaiNextHopGroupMemberTraits::Attributes::
                  NextHopId{});
      if (!isEgressToIp(ip, nextHopId)) {
        continue;
      }
      if (!weight) {
        return true;
      }
      return weight.value() ==
          SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
              static_cast<facebook::fboss::NextHopGroupMemberSaiId>(member),
              facebook::fboss::SaiNextHopGroupMemberTraits::Attributes::
                  Weight{});
    }
  } catch (const facebook::fboss::SaiApiError&) {
    return isEgressToIp(ip, nhop);
  }
  return false;
}

bool isRoutePresent(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch);

  auto routeAdapterKey = getSaiRouteAdapterKey(saiSwitch, rid, cidrNetwork);
  try {
    SaiApiTable::getInstance()->routeApi().getAttribute(
        routeAdapterKey, SaiRouteTraits::Attributes::NextHopId());
  } catch (const facebook::fboss::SaiApiError&) {
    return false;
  }

  return true;
}

bool isRouteUnresolvedToCpuClassId(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork) {
  auto classId = getHwRouteClassID(hwSwitch, rid, cidrNetwork);
  return classId.has_value() &&
      facebook::fboss::cfg::AclLookupClass(classId.value()) ==
      facebook::fboss::cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2;
}

} // namespace

namespace facebook {
namespace fboss {
namespace utility {

void HwTestThriftHandler::getRouteInfo(
    RouteInfo& routeInfo,
    std::unique_ptr<IpPrefix> prefix) {
  auto routePrefix = folly::CIDRNetwork(
      network::toIPAddress(*prefix->ip()), *prefix->prefixLength());
  routeInfo.exists() = isRoutePresent(hwSwitch_, RouterID(0), routePrefix);
  if (*routeInfo.exists()) {
    auto classID = getHwRouteClassID(hwSwitch_, RouterID(0), routePrefix);
    if (classID.has_value()) {
      routeInfo.classId() = classID.value();
    }
    routeInfo.isProgrammedToCpu() =
        isHwRouteToCpu(hwSwitch_, RouterID(0), routePrefix);
    routeInfo.isMultiPath() =
        isHwRouteMultiPath(hwSwitch_, RouterID(0), routePrefix);
    routeInfo.isRouteUnresolvedToClassId() =
        isRouteUnresolvedToCpuClassId(hwSwitch_, RouterID(0), routePrefix);
  }
}

bool HwTestThriftHandler::isRouteHit(std::unique_ptr<IpPrefix> prefix) {
  auto routePrefix = folly::CIDRNetwork(
      network::toIPAddress(*prefix->ip()), *prefix->prefixLength());
  return isHwRouteHit(hwSwitch_, RouterID(0), routePrefix);
}

void HwTestThriftHandler::clearRouteHit(std::unique_ptr<IpPrefix> prefix) {
  auto routePrefix = folly::CIDRNetwork(
      network::toIPAddress(*prefix->ip()), *prefix->prefixLength());
  clearHwRouteHit(hwSwitch_, RouterID(0), routePrefix);
}

bool HwTestThriftHandler::isRouteToNexthop(
    std::unique_ptr<IpPrefix> prefix,
    std::unique_ptr<network::thrift::BinaryAddress> nexthop) {
  auto routePrefix = folly::CIDRNetwork(
      network::toIPAddress(*prefix->ip()), *prefix->prefixLength());
  return isHwRouteToNextHop(
      hwSwitch_,
      RouterID(0),
      routePrefix,
      network::toIPAddress(*nexthop),
      std::nullopt);
}

} // namespace utility
} // namespace fboss
} // namespace facebook
