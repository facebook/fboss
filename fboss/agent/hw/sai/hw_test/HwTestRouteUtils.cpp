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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/RouteApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

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
    throw FbossError("No virtual router with id 0");
  }

  return SaiRouteTraits::RouteEntry(
      saiSwitch->getSaiSwitchId(),
      virtualRouterHandle->virtualRouter->adapterKey(),
      prefix);
}

std::optional<cfg::AclLookupClass> getHwRouteClassID(
    const HwSwitch* hw,
    RouterID rid,
    const folly::CIDRNetwork& prefix) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hw);

  auto r = getSaiRouteAdapterKey(saiSwitch, rid, prefix);
  auto metadata = SaiApiTable::getInstance()->routeApi().getAttribute(
      r, SaiRouteTraits::Attributes::Metadata());
  return metadata == 0 ? std::nullopt
                       : std::optional(cfg::AclLookupClass(metadata));
}

bool isHwRouteToCpu(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch);

  auto routeAdapterKey = getSaiRouteAdapterKey(saiSwitch, rid, cidrNetwork);
  sai_object_id_t nhop = SaiApiTable::getInstance()->routeApi().getAttribute(
      routeAdapterKey, SaiRouteTraits::Attributes::NextHopId());

  SwitchSaiId switchId =
      saiSwitch->managerTable()->switchManager().getSwitchSaiId();
  sai_object_id_t cpuPortId{
      SaiApiTable::getInstance()->switchApi().getAttribute(
          switchId, SaiSwitchTraits::Attributes::CpuPort{})};

  return nhop == cpuPortId;
}
bool isHwRouteHit(
    FOLLY_MAYBE_UNUSED const HwSwitch* hwSwitch,
    FOLLY_MAYBE_UNUSED RouterID /*rid*/,
    FOLLY_MAYBE_UNUSED const folly::CIDRNetwork& cidrNetwork) {
  throw FbossError("L3 entry hitbit is unsupported for SAI");
}

void clearHwRouteHit(
    FOLLY_MAYBE_UNUSED const HwSwitch* hwSwitch,
    FOLLY_MAYBE_UNUSED RouterID /*rid*/,
    FOLLY_MAYBE_UNUSED const folly::CIDRNetwork& cidrNetwork) {
  throw FbossError("L3 entry hitbit is unsupported for SAI");
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
        static_cast<NextHopGroupSaiId>(nhop),
        SaiNextHopGroupTraits::Attributes::NextHopMemberList{});
  } catch (const SaiApiError& err) {
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

  auto routeAdapterKey = getSaiRouteAdapterKey(saiSwitch, rid, cidrNetwork);
  sai_object_id_t nhop = SaiApiTable::getInstance()->routeApi().getAttribute(
      routeAdapterKey, SaiRouteTraits::Attributes::NextHopId());

  try {
    auto members = SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
        static_cast<NextHopGroupSaiId>(nhop),
        SaiNextHopGroupTraits::Attributes::NextHopMemberList{});
    for (auto member : members) {
      sai_object_id_t nextHopId =
          SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
              static_cast<NextHopGroupMemberSaiId>(member),
              SaiNextHopGroupMemberTraits::Attributes::NextHopId{});
      if (!isEgressToIp(ip, nextHopId)) {
        continue;
      }
      if (!weight) {
        return true;
      }
      return weight.value() ==
          SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
              static_cast<NextHopGroupMemberSaiId>(member),
              SaiNextHopGroupMemberTraits::Attributes::Weight{});
    }
  } catch (const SaiApiError& err) {
    return isEgressToIp(ip, nhop);
  }
  return false;
}

uint64_t getRouteStat(
    const HwSwitch* hwSwitch,
    std::optional<RouteCounterID> counterID) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch);
  auto retries = 10;
  auto initialStat =
      saiSwitch->managerTable()->counterManager().getStats(*counterID);
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto currentStat =
        saiSwitch->managerTable()->counterManager().getStats(*counterID);
    if (initialStat != currentStat) {
      return currentStat;
    }
  } while (retries-- > 0);
  return initialStat;
}

bool isHwRoutePresent(
    const HwSwitch* hwSwitch,
    RouterID rid,
    const folly::CIDRNetwork& cidrNetwork) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch);

  auto routeAdapterKey = getSaiRouteAdapterKey(saiSwitch, rid, cidrNetwork);
  try {
    SaiApiTable::getInstance()->routeApi().getAttribute(
        routeAdapterKey, SaiRouteTraits::Attributes::NextHopId());
  } catch (const SaiApiError& err) {
    return false;
  }

  return true;
}
} // namespace facebook::fboss::utility
