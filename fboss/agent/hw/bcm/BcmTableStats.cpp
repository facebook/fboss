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

#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>
#include <array>
#include "fboss/agent/hw/bcm/gen-cpp2/bcmswitch_constants.h"

extern "C" {
#include <bcm/field.h>
#include <bcm/l3.h>
#include <bcm/switch.h>
}

DECLARE_int32(acl_gid);

namespace facebook::fboss {

namespace {
constexpr auto kPerIpv6Mask65_127SlotUsage = 2;
}

bool BcmHwTableStatManager::refreshHwStatusStats(BcmHwTableStats* stats) {
  // HW status info
  bcm_l3_info_t l3HwStatus;
  auto ret = bcm_l3_info(hw_->getUnit(), &l3HwStatus);
  if (ret) {
    XLOG(ERR) << "Unable to get HW status, host table and ipv4 LPM"
                 " stats will be stale";
    return false;
  }
  stats->l3_host_max = std::max(0, l3HwStatus.l3info_max_host);
  stats->l3_host_used = std::max(0, l3HwStatus.l3info_used_host);
  stats->l3_host_free = std::max(0, stats->l3_host_max - stats->l3_host_used);
  stats->l3_nexthops_max = std::max(0, l3HwStatus.l3info_max_nexthop);
  stats->l3_nexthops_used = std::max(0, l3HwStatus.l3info_used_nexthop);
  stats->l3_nexthops_free =
      std::max(0, stats->l3_nexthops_max - stats->l3_nexthops_used);
  stats->l3_ipv4_host_used = std::max(0, l3HwStatus.l3info_used_host_ip4);
  stats->l3_ipv6_host_used = std::max(0, l3HwStatus.l3info_used_host_ip6);
  stats->l3_ecmp_groups_max = std::max(0, l3HwStatus.l3info_max_ecmp_groups);
  stats->l3_ecmp_groups_used =
      hw_->getMultiPathNextHopTable()->getEcmpEgressCount();
  stats->l3_ecmp_groups_free =
      std::max(0, stats->l3_ecmp_groups_max - stats->l3_ecmp_groups_used);
  return true;
}

bool BcmHwTableStatManager::refreshLPMStats(BcmHwTableStats* stats) {
  // IPv6 LPM table info
  std::array<int, 6> routeSlots = {0, 0, 0, 0, 0, 0};
  std::array<bcm_switch_object_t, routeSlots.size()> bcmFreeEntryTypes = {
      bcmSwitchObjectL3RouteV4RoutesMax,
      bcmSwitchObjectL3RouteV6Routes64bMax,
      bcmSwitchObjectL3RouteV6Routes128bMax,
      bcmSwitchObjectL3RouteV4RoutesUsed,
      bcmSwitchObjectL3RouteV6Routes64bUsed,
      bcmSwitchObjectL3RouteV6Routes128bUsed,
  };

  auto ret = bcm_switch_object_count_multi_get(
      hw_->getUnit(),
      routeSlots.size(),
      bcmFreeEntryTypes.data(),
      routeSlots.data());
  if (ret) {
    XLOG(ERR) << "Unable to get LPM table usage, LPM table "
                 "stats will be stale";
    return false;
  }
  // Max
  stats->lpm_ipv4_max = routeSlots[0];
  stats->lpm_ipv6_mask_0_64_max = routeSlots[1];
  stats->lpm_ipv6_mask_65_127_max = routeSlots[2];
  // Used
  stats->lpm_ipv4_used = routeSlots[3];
  stats->lpm_ipv6_mask_0_64_used = routeSlots[4];
  stats->lpm_ipv6_mask_65_127_used = routeSlots[5];

  // Ideally lpm slots max and used should come from SDK.
  // However SDK always has these set to 0 in bcm_l3_info_t.
  // There is a case open with BCM for this. If we get a patch
  // or newer SDK fixes this, we can get rid of the calculation
  // below.
  // Note that v4 and v6 /0-/64 share same TCAM area so we
  // only need to count them once.
  stats->lpm_slots_max = stats->lpm_ipv6_mask_0_64_max +
      stats->lpm_ipv6_mask_65_127_max * kPerIpv6Mask65_127SlotUsage;

  return true;
}

bool BcmHwTableStatManager::refreshLPMOnlyStats(BcmHwTableStats* stats) {
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
  stats->lpm_ipv4_free = routeSlots[0];
  stats->lpm_ipv6_mask_0_64_free = routeSlots[1];
  stats->lpm_ipv6_mask_65_127_free = routeSlots[2];

  stats->lpm_slots_free = stats->lpm_ipv6_mask_0_64_free +
      stats->lpm_ipv6_mask_65_127_free * kPerIpv6Mask65_127SlotUsage;
  // refreshLPMStats must be called first
  stats->lpm_slots_used = stats->lpm_slots_max - stats->lpm_slots_free;
  return true;
}

bool BcmHwTableStatManager::refreshFPStats(BcmHwTableStats* stats) {
  bcm_field_group_status_t aclStatus;
  auto ret =
      bcm_field_group_status_get(hw_->getUnit(), FLAGS_acl_gid, &aclStatus);
  if (ret) {
    return false;
  }
  // Entries
  stats->acl_entries_used = aclStatus.entry_count;
  stats->acl_entries_max = aclStatus.entries_total;
  stats->acl_entries_free = aclStatus.entries_free;
  // Counters
  stats->acl_counters_used = aclStatus.counter_count;
  stats->acl_counters_free = aclStatus.counters_free;
  // compute max via used + free rather than using counters total
  // The latter is higher than what is available to this group
  stats->acl_counters_max = stats->acl_counters_used + stats->acl_counters_free;
  // Meters
  stats->acl_meters_used = aclStatus.meter_count;
  stats->acl_meters_free = aclStatus.meters_free;
  stats->acl_meters_max = aclStatus.meters_total;
  return true;
}

void BcmHwTableStatManager::updateBcmStateChangeStats(
    const StateDelta& delta,
    BcmHwTableStats* stats) {
  if (stats->mirrors_erspan == bcmswitch_constants::STAT_UNINITIALIZED_) {
    stats->mirrors_erspan = 0;
  }
  if (stats->mirrors_span == bcmswitch_constants::STAT_UNINITIALIZED_) {
    stats->mirrors_span = 0;
  }
  if (stats->mirrors_sflow == bcmswitch_constants::STAT_UNINITIALIZED_) {
    stats->mirrors_sflow = 0;
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
  stats->mirrors_max = bcmswitch_constants::MAX_MIRRORS_;
  stats->mirrors_used =
      stats->mirrors_erspan + stats->mirrors_span + stats->mirrors_sflow;
  stats->mirrors_free = stats->mirrors_max - stats->mirrors_used;
}

void BcmHwTableStatManager::decrementBcmMirrorStat(
    const std::shared_ptr<Mirror>& removedMirror,
    BcmHwTableStats* stats) {
  CHECK(removedMirror->isResolved());
  auto tunnel = removedMirror->getMirrorTunnel();
  if (!tunnel) {
    stats->mirrors_span--;
  } else if (tunnel && !tunnel->udpPorts) {
    stats->mirrors_erspan--;
  } else if (tunnel && tunnel->udpPorts) {
    stats->mirrors_sflow--;
  }
}

void BcmHwTableStatManager::incrementBcmMirrorStat(
    const std::shared_ptr<Mirror>& addedMirror,
    BcmHwTableStats* stats) {
  CHECK(addedMirror->isResolved());
  auto tunnel = addedMirror->getMirrorTunnel();
  if (!tunnel) {
    stats->mirrors_span++;
  } else if (tunnel && !tunnel->udpPorts) {
    stats->mirrors_erspan++;
  } else if (tunnel && tunnel->udpPorts) {
    stats->mirrors_sflow++;
  }
}

void BcmHwTableStatManager::publish(BcmHwTableStats stats) const {
  fb303::fbData->setCounter(
      "hw_table_stats_stale", stats.hw_table_stats_stale ? 1 : 0);
  fb303::fbData->setCounter("l3_host_max", stats.l3_host_max);
  fb303::fbData->setCounter("l3_host_used", stats.l3_host_used);
  fb303::fbData->setCounter("l3_host_free", stats.l3_host_free);
  fb303::fbData->setCounter("l3_nexthops_max", stats.l3_nexthops_max);
  fb303::fbData->setCounter("l3_nexthops_used", stats.l3_nexthops_used);
  fb303::fbData->setCounter("l3_nexthops_free", stats.l3_nexthops_free);
  fb303::fbData->setCounter("l3_ecmp_groups_used", stats.l3_ecmp_groups_used);

  fb303::fbData->setCounter("l3_ipv4_host_used", stats.l3_ipv4_host_used);
  fb303::fbData->setCounter("l3_ipv6_host_used", stats.l3_ipv6_host_used);
  fb303::fbData->setCounter("l3_ecmp_groups_max", stats.l3_ecmp_groups_max);
  fb303::fbData->setCounter("l3_ecmp_groups_free", stats.l3_ecmp_groups_free);

  // LPM
  fb303::fbData->setCounter("lpm_ipv4_max", stats.lpm_ipv4_max);
  fb303::fbData->setCounter("lpm_ipv4_free", stats.lpm_ipv4_free);
  fb303::fbData->setCounter("lpm_ipv4_used", stats.lpm_ipv4_used);
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_max", stats.lpm_ipv6_mask_0_64_max);
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_free", stats.lpm_ipv6_mask_0_64_free);
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_used", stats.lpm_ipv6_mask_0_64_used);
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_65_127_max", stats.lpm_ipv6_mask_65_127_max);
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_free", stats.lpm_ipv6_mask_0_64_free);
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_65_127_free", stats.lpm_ipv6_mask_65_127_free);
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_65_127_used", stats.lpm_ipv6_mask_65_127_used);
  fb303::fbData->setCounter("lpm_table_max", stats.lpm_slots_max);
  fb303::fbData->setCounter("lpm_table_used", stats.lpm_slots_used);
  fb303::fbData->setCounter("lpm_table_free", stats.lpm_slots_free);
  // Acl
  fb303::fbData->setCounter("acl_entries_used", stats.acl_entries_used);
  fb303::fbData->setCounter("acl_entries_free", stats.acl_entries_free);
  fb303::fbData->setCounter("acl_entries_max", stats.acl_entries_max);
  fb303::fbData->setCounter("acl_counters_used", stats.acl_counters_used);
  fb303::fbData->setCounter("acl_counters_free", stats.acl_counters_free);
  fb303::fbData->setCounter("acl_counters_max", stats.acl_counters_max);
  fb303::fbData->setCounter("acl_meters_used", stats.acl_meters_used);
  fb303::fbData->setCounter("acl_meters_free", stats.acl_meters_free);
  fb303::fbData->setCounter("acl_meters_max", stats.acl_meters_max);
  // Mirrors
  fb303::fbData->setCounter("mirrors_used", stats.mirrors_used);
  fb303::fbData->setCounter("mirrors_free", stats.mirrors_free);
  fb303::fbData->setCounter("mirrors_max", stats.mirrors_max);
  fb303::fbData->setCounter("mirrors_span", stats.mirrors_span);
  fb303::fbData->setCounter("mirrors_erspan", stats.mirrors_erspan);
  fb303::fbData->setCounter("mirrors_sflow", stats.mirrors_sflow);
}

void BcmHwTableStatManager::refresh(
    const StateDelta& delta,
    BcmHwTableStats* stats) {
  stats->hw_table_stats_stale =
      !(refreshHwStatusStats(stats) && refreshLPMStats(stats) &&
        refreshFPStats(stats));
  if (!isAlpmEnabled_) {
    stats->hw_table_stats_stale |= !(refreshLPMOnlyStats(stats));
  }
  updateBcmStateChangeStats(delta, stats);
}

} // namespace facebook::fboss
