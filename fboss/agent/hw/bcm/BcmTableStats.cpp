/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTableStats.h"

#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmExactMatchUtils.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include <folly/logging/xlog.h>
#include <array>
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"

extern "C" {
#include <bcm/field.h>
#include <bcm/l3.h>
#include <bcm/switch.h>
}

namespace facebook::fboss {

using namespace facebook::fboss::utility;

namespace {
constexpr auto kPerIpv6Mask65_127SlotUsage = 2;
}

BcmHwTableStatManager::BcmHwTableStatManager(const BcmSwitch* hw)
    : hw_(hw),
      isAlpmEnabled_(BcmAPI::isAlpmEnabled()),
      is128ByteIpv6Enabled_(BcmAPI::is128ByteIpv6Enabled()) {}

bool BcmHwTableStatManager::refreshHwStatusStats(HwResourceStats* stats) const {
  // HW status info
  bcm_l3_info_t l3HwStatus;
  auto ret = bcm_l3_info(hw_->getUnit(), &l3HwStatus);
  if (ret) {
    XLOG(ERR) << "Unable to get HW status, host table and ipv4 LPM"
                 " stats will be stale";
    return false;
  }
  stats->l3_nexthops_max() = std::max(0, l3HwStatus.l3info_max_nexthop);
  stats->l3_nexthops_used() = std::max(0, l3HwStatus.l3info_used_nexthop);
  stats->l3_nexthops_free() =
      std::max(0, *stats->l3_nexthops_max() - *stats->l3_nexthops_used());
  // Nexthops correspond to egresses, which are shared (and thus same) for
  // v4 and v6
  stats->l3_ipv4_nexthops_free() = *stats->l3_nexthops_free();
  stats->l3_ipv6_nexthops_free() = *stats->l3_nexthops_free();
  stats->l3_ecmp_groups_max() = std::max(0, l3HwStatus.l3info_max_ecmp_groups);
  stats->l3_ecmp_groups_used() =
      hw_->getMultiPathNextHopTable()->getEcmpEgressCount();
  stats->l3_ecmp_groups_free() =
      std::max(0, *stats->l3_ecmp_groups_max() - *stats->l3_ecmp_groups_used());
  if (!hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::HOSTTABLE)) {
    stats->l3_host_max() = 0;
    stats->l3_host_used() = 0;
    stats->l3_host_free() = 0;
    stats->l3_ipv4_host_used() = 0;
    stats->l3_ipv4_host_free() = 0;
    stats->l3_ipv6_host_used() = 0;
    stats->l3_ipv6_host_free() = 0;
    return true;
  }
  // Get v4, v6 host counts
  stats->l3_host_max() = std::max(0, l3HwStatus.l3info_max_host);
  stats->l3_host_used() = std::max(0, l3HwStatus.l3info_used_host);
  stats->l3_host_free() =
      std::max(0, *stats->l3_host_max() - *stats->l3_host_used());
  stats->l3_ipv4_host_used() = std::max(0, l3HwStatus.l3info_used_host_ip4);
  stats->l3_ipv6_host_used() = std::max(0, l3HwStatus.l3info_used_host_ip6);
  int v4Max;
  auto v4Stale =
      bcm_switch_object_count_get(0, bcmSwitchObjectL3HostV4Max, &v4Max);
  if (!v4Stale) {
    stats->l3_ipv4_host_free() = std::max(0, v4Max - *stats->l3_host_used());
  }
  int v6Max;
  auto v6Stale =
      bcm_switch_object_count_get(0, bcmSwitchObjectL3HostV6Max, &v6Max);
  if (!v6Stale) {
    // v6 hosts take 2 slots each
    stats->l3_ipv6_host_free() =
        std::max(0, (v6Max - *stats->l3_host_used() / 2));
  }
  return !v4Stale && !v6Stale;
}

int BcmHwTableStatManager::getAlpmUsedRouteCounts(
    AlpmUsedRouteCounts& counts) const {
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::ALPM_ROUTE_PROJECTION)) {
    if (int ret = getAlpmUsedRouteCount(counts[kIpv4AlpmUsedIndex], false)) {
      return ret;
    }
    if (is128ByteIpv6Enabled_) {
      counts[kIpv6Mask64AlpmUsedIndex] = 0;
      return getAlpmUsedRouteCount(counts[kIpv6Mask128AlpmUsedIndex], true);
    } else {
      counts[kIpv6Mask128AlpmUsedIndex] = 0;
      return getAlpmUsedRouteCount(counts[kIpv6Mask64AlpmUsedIndex], true);
    }
  }
  std::array<bcm_switch_object_t, 3> bcmUsedEntryTypes = {
      bcmSwitchObjectL3RouteV4RoutesUsed,
      bcmSwitchObjectL3RouteV6Routes64bUsed,
      bcmSwitchObjectL3RouteV6Routes128bUsed,
  };

  return bcm_switch_object_count_multi_get(
      hw_->getUnit(), counts.size(), bcmUsedEntryTypes.data(), counts.data());
}

bool BcmHwTableStatManager::refreshLPMStats(HwResourceStats* stats) const {
  // IPv6 LPM table info
  // Used
  std::array<int, 3> counts = {0, 0, 0};
  auto ret = getAlpmUsedRouteCounts(counts);
  if (ret) {
    XLOG(ERR) << "Unable to get LPM route table used";
    return false;
  }
  stats->lpm_ipv4_used() = counts[kIpv4AlpmUsedIndex];
  stats->lpm_ipv6_mask_0_64_used() = counts[kIpv6Mask64AlpmUsedIndex];
  stats->lpm_ipv6_mask_65_127_used() = counts[kIpv6Mask128AlpmUsedIndex];

  // Max
  int v4Max, v6Max;
  ret = getAlpmMaxRouteCount(v4Max, false);
  if (ret) {
    XLOG(ERR) << "Unable to get v4 LPM table max";
    return false;
  }
  stats->lpm_ipv4_max() = v4Max;
  ret = getAlpmMaxRouteCount(v6Max, true);
  if (ret) {
    XLOG(ERR) << "Unable to get v6 LPM table max";
    return false;
  }
  if (is128ByteIpv6Enabled_) {
    stats->lpm_ipv6_mask_0_64_max() = 0;
    stats->lpm_ipv6_mask_65_127_max() = v6Max;
  } else {
    stats->lpm_ipv6_mask_0_64_max() = v6Max;
    stats->lpm_ipv6_mask_65_127_max() = 0;
  }

  // Ideally lpm slots max and used should come from SDK.
  // However SDK always has these set to 0 in bcm_l3_info_t.
  // There is a case open with BCM for this. If we get a patch
  // or newer SDK fixes this, we can get rid of the calculation
  // below.
  // Note that v4 and v6 /0-/64 share same TCAM area so we
  // only need to count them once.
  stats->lpm_slots_max() = *stats->lpm_ipv6_mask_0_64_max() +
      *stats->lpm_ipv6_mask_65_127_max() * kPerIpv6Mask65_127SlotUsage;

  return true;
}

bool BcmHwTableStatManager::refreshLPMOnlyStats(HwResourceStats* stats) const {
  // IPv6 LPM table info
  std::array<int, 3> routeSlots = {0, 0, 0};
  std::array<bcm_switch_object_t, routeSlots.size()> bcmFreeEntryTypes = {
      bcmSwitchObjectL3RouteV4RoutesFree,
      bcmSwitchObjectL3RouteV6Routes64bFree,
      bcmSwitchObjectL3RouteV6Routes128bFree,
  };

  auto ret = bcm_switch_object_count_multi_get(
      hw_->getUnit(),
      routeSlots.size(),
      bcmFreeEntryTypes.data(),
      routeSlots.data());
  if (ret) {
    XLOG(ERR) << "Unable to get LPM free stats, these "
                 "will be stale";
    return false;
  }
  // Free
  stats->lpm_ipv4_free() = routeSlots[0];
  stats->lpm_ipv6_mask_0_64_free() = routeSlots[1];
  stats->lpm_ipv6_mask_65_127_free() = routeSlots[2];
  stats->lpm_ipv6_free() = routeSlots[1] + routeSlots[2];

  stats->lpm_slots_free() = *stats->lpm_ipv6_mask_0_64_free() +
      *stats->lpm_ipv6_mask_65_127_free() * kPerIpv6Mask65_127SlotUsage;
  // refreshLPMStats must be called first
  stats->lpm_slots_used() = *stats->lpm_slots_max() - *stats->lpm_slots_free();
  return true;
}

bool BcmHwTableStatManager::refreshAlpmFreeRouteCounts(
    HwResourceStats* stats) const {
  bool v4Stale{false}, v6Stale{false};
  {
    // v4
    int v4Max, v4Used;
    auto ret1 = getAlpmMaxRouteCount(v4Max, false);
    auto ret2 = getAlpmUsedRouteCount(v4Used, false);
    if (!ret1 && !ret2) {
      stats->lpm_ipv4_free() = std::max(0, v4Max - v4Used);
    } else {
      v4Stale = true;
    }
  }
  {
    // v6
    int v6Max, v6Used;
    int ret1, ret2;
    ret1 = getAlpmMaxRouteCount(v6Max, true);
    ret2 = getAlpmUsedRouteCount(v6Used, true);
    if (!ret1 && !ret2) {
      stats->lpm_ipv6_free() = std::max(0, v6Max - v6Used);
    } else {
      v6Stale = true;
    }
  }
  return !v4Stale && !v6Stale;
}

bool BcmHwTableStatManager::refreshFPStats(HwResourceStats* stats) const {
  bcm_field_group_status_t aclStatus;
  auto ret = bcm_field_group_status_get(
      hw_->getUnit(),
      hw_->getPlatform()->getAsic()->getDefaultACLGroupID(),
      &aclStatus);
  if (ret) {
    XLOG(ERR) << "Unable to get ACL stats, these will be stale";
    return false;
  }
  // Entries
  stats->acl_entries_used() = aclStatus.entry_count;
  stats->acl_entries_max() = aclStatus.entries_total;
  stats->acl_entries_free() = aclStatus.entries_free;
  // Counters
  stats->acl_counters_used() = aclStatus.counter_count;
  stats->acl_counters_free() = aclStatus.counters_free;
  // compute max via used + free rather than using counters total
  // The latter is higher than what is available to this group
  stats->acl_counters_max() =
      *stats->acl_counters_used() + *stats->acl_counters_free();
  // Meters
  stats->acl_meters_used() = aclStatus.meter_count;
  stats->acl_meters_free() = aclStatus.meters_free;
  stats->acl_meters_max() = aclStatus.meters_total;
  return true;
}

bool BcmHwTableStatManager::refreshEMStats(HwResourceStats* stats) const {
  auto grpId = hw_->getTeFlowTable()->getTeFlowGroupId();
  if (!grpId) {
    stats->em_entries_used() = 0;
    stats->em_entries_max() = 0;
    stats->em_entries_free() = 0;
    stats->em_counters_used() = 0;
    stats->em_counters_free() = 0;
    stats->em_counters_max() = 0;
    return true;
  }
  bcm_field_group_status_t emStatus;
  auto ret = bcm_field_group_status_get(
      hw_->getUnit(), getEMGroupID(grpId), &emStatus);
  if (ret) {
    XLOG(ERR) << "Unable to get EM stats, these will be stale";
    return false;
  }
  // Entries
  stats->em_entries_used() = emStatus.entry_count;
  stats->em_entries_max() = emStatus.entries_total;
  stats->em_entries_free() = emStatus.entries_free;
  // Counters
  stats->em_counters_used() = emStatus.counter_count;
  stats->em_counters_free() = emStatus.counters_free;
  // compute max via used + free rather than using counters total
  // The latter is higher than what is available to this group
  stats->em_counters_max() =
      *stats->em_counters_used() + *stats->em_counters_free();
  return true;
}

void BcmHwTableStatManager::updateBcmStateChangeStats(
    const StateDelta& delta,
    HwResourceStats* stats) const {
  if (*stats->mirrors_erspan() ==
      hardware_stats_constants::STAT_UNINITIALIZED()) {
    stats->mirrors_erspan() = 0;
  }
  if (*stats->mirrors_span() ==
      hardware_stats_constants::STAT_UNINITIALIZED()) {
    stats->mirrors_span() = 0;
  }
  if (*stats->mirrors_sflow() ==
      hardware_stats_constants::STAT_UNINITIALIZED()) {
    stats->mirrors_sflow() = 0;
  }
  DeltaFunctions::forEachChanged(
      delta.getMirrorsDelta(),
      [=](const std::shared_ptr<Mirror>& oldMirror,
          const std::shared_ptr<Mirror>& newMirror) {
        if (oldMirror->isResolved()) {
          decrementBcmMirrorStat(oldMirror, stats);
        }
        if (newMirror->isResolved()) {
          incrementBcmMirrorStat(newMirror, stats);
        }
      },
      [=](const std::shared_ptr<Mirror>& addedMirror) {
        if (addedMirror->isResolved()) {
          incrementBcmMirrorStat(addedMirror, stats);
        }
      },
      [=](const std::shared_ptr<Mirror>& removedMirror) {
        if (removedMirror->isResolved()) {
          decrementBcmMirrorStat(removedMirror, stats);
        }
      });
  stats->mirrors_max() = hw_->getPlatform()->getAsic()->getMaxMirrors();
  stats->mirrors_used() = *stats->mirrors_erspan() + *stats->mirrors_span() +
      *stats->mirrors_sflow();
  stats->mirrors_free() = *stats->mirrors_max() - *stats->mirrors_used();
}

void BcmHwTableStatManager::decrementBcmMirrorStat(
    const std::shared_ptr<Mirror>& removedMirror,
    HwResourceStats* stats) const {
  CHECK(removedMirror->isResolved());
  auto tunnel = removedMirror->getMirrorTunnel();
  if (!tunnel) {
    (*stats->mirrors_span())--;
  } else if (tunnel && !tunnel->udpPorts) {
    (*stats->mirrors_erspan())--;
  } else if (tunnel && tunnel->udpPorts) {
    (*stats->mirrors_sflow())--;
  }
}

void BcmHwTableStatManager::incrementBcmMirrorStat(
    const std::shared_ptr<Mirror>& addedMirror,
    HwResourceStats* stats) const {
  CHECK(addedMirror->isResolved());
  auto tunnel = addedMirror->getMirrorTunnel();
  if (!tunnel) {
    (*stats->mirrors_span())++;
  } else if (tunnel && !tunnel->udpPorts) {
    (*stats->mirrors_erspan())++;
  } else if (tunnel && tunnel->udpPorts) {
    (*stats->mirrors_sflow())++;
  }
}

void BcmHwTableStatManager::refresh(
    const StateDelta& delta,
    HwResourceStats* stats) const {
  auto hwStatus = refreshHwStatusStats(stats);
  auto lpmStatus = refreshLPMStats(stats);
  auto fpStatus = refreshFPStats(stats);
  auto emStatus = refreshEMStats(stats);
  stats->hw_table_stats_stale() =
      !(hwStatus && lpmStatus && fpStatus && emStatus);
  if (!isAlpmEnabled_) {
    *stats->hw_table_stats_stale() |= !(refreshLPMOnlyStats(stats));
  } else {
    *stats->hw_table_stats_stale() |= !(refreshAlpmFreeRouteCounts(stats));
  }
  updateBcmStateChangeStats(delta, stats);
}

} // namespace facebook::fboss
