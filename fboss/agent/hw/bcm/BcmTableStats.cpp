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
#include "fboss/agent/hw/switch_asics/HwAsic.h"

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
  *stats->l3_host_max_ref() = std::max(0, l3HwStatus.l3info_max_host);
  *stats->l3_host_used_ref() = std::max(0, l3HwStatus.l3info_used_host);
  *stats->l3_host_free_ref() =
      std::max(0, *stats->l3_host_max_ref() - *stats->l3_host_used_ref());
  *stats->l3_nexthops_max_ref() = std::max(0, l3HwStatus.l3info_max_nexthop);
  *stats->l3_nexthops_used_ref() = std::max(0, l3HwStatus.l3info_used_nexthop);
  *stats->l3_nexthops_free_ref() = std::max(
      0, *stats->l3_nexthops_max_ref() - *stats->l3_nexthops_used_ref());
  *stats->l3_ipv4_host_used_ref() =
      std::max(0, l3HwStatus.l3info_used_host_ip4);
  *stats->l3_ipv6_host_used_ref() =
      std::max(0, l3HwStatus.l3info_used_host_ip6);
  *stats->l3_ecmp_groups_max_ref() =
      std::max(0, l3HwStatus.l3info_max_ecmp_groups);
  *stats->l3_ecmp_groups_used_ref() =
      hw_->getMultiPathNextHopTable()->getEcmpEgressCount();
  *stats->l3_ecmp_groups_free_ref() = std::max(
      0, *stats->l3_ecmp_groups_max_ref() - *stats->l3_ecmp_groups_used_ref());
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
  *stats->lpm_ipv4_max_ref() = routeSlots[0];
  *stats->lpm_ipv6_mask_0_64_max_ref() = routeSlots[1];
  *stats->lpm_ipv6_mask_65_127_max_ref() = routeSlots[2];
  // Used
  *stats->lpm_ipv4_used_ref() = routeSlots[3];
  *stats->lpm_ipv6_mask_0_64_used_ref() = routeSlots[4];
  *stats->lpm_ipv6_mask_65_127_used_ref() = routeSlots[5];

  // Ideally lpm slots max and used should come from SDK.
  // However SDK always has these set to 0 in bcm_l3_info_t.
  // There is a case open with BCM for this. If we get a patch
  // or newer SDK fixes this, we can get rid of the calculation
  // below.
  // Note that v4 and v6 /0-/64 share same TCAM area so we
  // only need to count them once.
  *stats->lpm_slots_max_ref() = *stats->lpm_ipv6_mask_0_64_max_ref() +
      *stats->lpm_ipv6_mask_65_127_max_ref() * kPerIpv6Mask65_127SlotUsage;

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
  *stats->lpm_ipv4_free_ref() = routeSlots[0];
  *stats->lpm_ipv6_mask_0_64_free_ref() = routeSlots[1];
  *stats->lpm_ipv6_mask_65_127_free_ref() = routeSlots[2];

  *stats->lpm_slots_free_ref() = *stats->lpm_ipv6_mask_0_64_free_ref() +
      *stats->lpm_ipv6_mask_65_127_free_ref() * kPerIpv6Mask65_127SlotUsage;
  // refreshLPMStats must be called first
  *stats->lpm_slots_used_ref() =
      *stats->lpm_slots_max_ref() - *stats->lpm_slots_free_ref();
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
  *stats->acl_entries_used_ref() = aclStatus.entry_count;
  *stats->acl_entries_max_ref() = aclStatus.entries_total;
  *stats->acl_entries_free_ref() = aclStatus.entries_free;
  // Counters
  *stats->acl_counters_used_ref() = aclStatus.counter_count;
  *stats->acl_counters_free_ref() = aclStatus.counters_free;
  // compute max via used + free rather than using counters total
  // The latter is higher than what is available to this group
  *stats->acl_counters_max_ref() =
      *stats->acl_counters_used_ref() + *stats->acl_counters_free_ref();
  // Meters
  *stats->acl_meters_used_ref() = aclStatus.meter_count;
  *stats->acl_meters_free_ref() = aclStatus.meters_free;
  *stats->acl_meters_max_ref() = aclStatus.meters_total;
  return true;
}

void BcmHwTableStatManager::updateBcmStateChangeStats(
    const StateDelta& delta,
    BcmHwTableStats* stats) {
  if (*stats->mirrors_erspan_ref() ==
      bcmswitch_constants::STAT_UNINITIALIZED_) {
    *stats->mirrors_erspan_ref() = 0;
  }
  if (*stats->mirrors_span_ref() == bcmswitch_constants::STAT_UNINITIALIZED_) {
    *stats->mirrors_span_ref() = 0;
  }
  if (*stats->mirrors_sflow_ref() == bcmswitch_constants::STAT_UNINITIALIZED_) {
    *stats->mirrors_sflow_ref() = 0;
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
  *stats->mirrors_max_ref() = hw_->getPlatform()->getAsic()->getMaxMirrors();
  *stats->mirrors_used_ref() = *stats->mirrors_erspan_ref() +
      *stats->mirrors_span_ref() + *stats->mirrors_sflow_ref();
  *stats->mirrors_free_ref() =
      *stats->mirrors_max_ref() - *stats->mirrors_used_ref();
}

void BcmHwTableStatManager::decrementBcmMirrorStat(
    const std::shared_ptr<Mirror>& removedMirror,
    BcmHwTableStats* stats) {
  CHECK(removedMirror->isResolved());
  auto tunnel = removedMirror->getMirrorTunnel();
  if (!tunnel) {
    (*stats->mirrors_span_ref())--;
  } else if (tunnel && !tunnel->udpPorts) {
    (*stats->mirrors_erspan_ref())--;
  } else if (tunnel && tunnel->udpPorts) {
    (*stats->mirrors_sflow_ref())--;
  }
}

void BcmHwTableStatManager::incrementBcmMirrorStat(
    const std::shared_ptr<Mirror>& addedMirror,
    BcmHwTableStats* stats) {
  CHECK(addedMirror->isResolved());
  auto tunnel = addedMirror->getMirrorTunnel();
  if (!tunnel) {
    (*stats->mirrors_span_ref())++;
  } else if (tunnel && !tunnel->udpPorts) {
    (*stats->mirrors_erspan_ref())++;
  } else if (tunnel && tunnel->udpPorts) {
    (*stats->mirrors_sflow_ref())++;
  }
}

void BcmHwTableStatManager::publish(BcmHwTableStats stats) const {
  fb303::fbData->setCounter(
      "hw_table_stats_stale", *stats.hw_table_stats_stale_ref() ? 1 : 0);
  fb303::fbData->setCounter("l3_host_max", *stats.l3_host_max_ref());
  fb303::fbData->setCounter("l3_host_used", *stats.l3_host_used_ref());
  fb303::fbData->setCounter("l3_host_free", *stats.l3_host_free_ref());
  fb303::fbData->setCounter("l3_nexthops_max", *stats.l3_nexthops_max_ref());
  fb303::fbData->setCounter("l3_nexthops_used", *stats.l3_nexthops_used_ref());
  fb303::fbData->setCounter("l3_nexthops_free", *stats.l3_nexthops_free_ref());
  fb303::fbData->setCounter(
      "l3_ecmp_groups_used", *stats.l3_ecmp_groups_used_ref());

  fb303::fbData->setCounter(
      "l3_ipv4_host_used", *stats.l3_ipv4_host_used_ref());
  fb303::fbData->setCounter(
      "l3_ipv6_host_used", *stats.l3_ipv6_host_used_ref());
  fb303::fbData->setCounter(
      "l3_ecmp_groups_max", *stats.l3_ecmp_groups_max_ref());
  fb303::fbData->setCounter(
      "l3_ecmp_groups_free", *stats.l3_ecmp_groups_free_ref());

  // LPM
  fb303::fbData->setCounter("lpm_ipv4_max", *stats.lpm_ipv4_max_ref());
  fb303::fbData->setCounter("lpm_ipv4_free", *stats.lpm_ipv4_free_ref());
  fb303::fbData->setCounter("lpm_ipv4_used", *stats.lpm_ipv4_used_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_max", *stats.lpm_ipv6_mask_0_64_max_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_free", *stats.lpm_ipv6_mask_0_64_free_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_used", *stats.lpm_ipv6_mask_0_64_used_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_65_127_max", *stats.lpm_ipv6_mask_65_127_max_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_0_64_free", *stats.lpm_ipv6_mask_0_64_free_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_65_127_free", *stats.lpm_ipv6_mask_65_127_free_ref());
  fb303::fbData->setCounter(
      "lpm_ipv6_mask_65_127_used", *stats.lpm_ipv6_mask_65_127_used_ref());
  fb303::fbData->setCounter("lpm_table_max", *stats.lpm_slots_max_ref());
  fb303::fbData->setCounter("lpm_table_used", *stats.lpm_slots_used_ref());
  fb303::fbData->setCounter("lpm_table_free", *stats.lpm_slots_free_ref());
  // Acl
  fb303::fbData->setCounter("acl_entries_used", *stats.acl_entries_used_ref());
  fb303::fbData->setCounter("acl_entries_free", *stats.acl_entries_free_ref());
  fb303::fbData->setCounter("acl_entries_max", *stats.acl_entries_max_ref());
  fb303::fbData->setCounter(
      "acl_counters_used", *stats.acl_counters_used_ref());
  fb303::fbData->setCounter(
      "acl_counters_free", *stats.acl_counters_free_ref());
  fb303::fbData->setCounter("acl_counters_max", *stats.acl_counters_max_ref());
  fb303::fbData->setCounter("acl_meters_used", *stats.acl_meters_used_ref());
  fb303::fbData->setCounter("acl_meters_free", *stats.acl_meters_free_ref());
  fb303::fbData->setCounter("acl_meters_max", *stats.acl_meters_max_ref());
  // Mirrors
  fb303::fbData->setCounter("mirrors_used", *stats.mirrors_used_ref());
  fb303::fbData->setCounter("mirrors_free", *stats.mirrors_free_ref());
  fb303::fbData->setCounter("mirrors_max", *stats.mirrors_max_ref());
  fb303::fbData->setCounter("mirrors_span", *stats.mirrors_span_ref());
  fb303::fbData->setCounter("mirrors_erspan", *stats.mirrors_erspan_ref());
  fb303::fbData->setCounter("mirrors_sflow", *stats.mirrors_sflow_ref());
}

void BcmHwTableStatManager::refresh(
    const StateDelta& delta,
    BcmHwTableStats* stats) {
  *stats->hw_table_stats_stale_ref() =
      !(refreshHwStatusStats(stats) && refreshLPMStats(stats) &&
        refreshFPStats(stats));
  if (!isAlpmEnabled_) {
    *stats->hw_table_stats_stale_ref() |= !(refreshLPMOnlyStats(stats));
  }
  updateBcmStateChangeStats(delta, stats);
}

} // namespace facebook::fboss
