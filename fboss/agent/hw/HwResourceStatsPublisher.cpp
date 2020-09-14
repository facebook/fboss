/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwResourceStatsPublisher.h"

#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

#include <fb303/ServiceData.h>

namespace facebook::fboss {

void HwResourceStatPublisher::publish(const HwResourceStats& stats) const {
  fb303::fbData->setCounter(
      kHwTableStatsStale, *stats.hw_table_stats_stale_ref() ? 1 : 0);
  fb303::fbData->setCounter(kL3HostMax, *stats.l3_host_max_ref());
  fb303::fbData->setCounter(kL3HostUsed, *stats.l3_host_used_ref());
  fb303::fbData->setCounter(kL3HostFree, *stats.l3_host_free_ref());
  fb303::fbData->setCounter(kL3NextHopsMax, *stats.l3_nexthops_max_ref());
  fb303::fbData->setCounter(kL3NextHopsUsed, *stats.l3_nexthops_used_ref());
  fb303::fbData->setCounter(kL3NextHopsFree, *stats.l3_nexthops_free_ref());
  fb303::fbData->setCounter(kL3EcmpGroupsMax, *stats.l3_ecmp_groups_max_ref());
  fb303::fbData->setCounter(
      kL3EcmpGroupsUsed, *stats.l3_ecmp_groups_used_ref());
  fb303::fbData->setCounter(
      kL3EcmpGroupsFree, *stats.l3_ecmp_groups_free_ref());

  fb303::fbData->setCounter(kL3Ipv4HostUsed, *stats.l3_ipv4_host_used_ref());
  fb303::fbData->setCounter(kL3Ipv6HostUsed, *stats.l3_ipv6_host_used_ref());

  // LPM
  fb303::fbData->setCounter(kLpmIpv4Max, *stats.lpm_ipv4_max_ref());
  fb303::fbData->setCounter(kLpmIpv4Used, *stats.lpm_ipv4_used_ref());
  fb303::fbData->setCounter(kLpmIpv4Free, *stats.lpm_ipv4_free_ref());
  fb303::fbData->setCounter(
      kLpmIpv6Mask_0_64_Max, *stats.lpm_ipv6_mask_0_64_max_ref());
  fb303::fbData->setCounter(
      kLpmIpv6Mask_0_64_Used, *stats.lpm_ipv6_mask_0_64_used_ref());
  fb303::fbData->setCounter(
      kLpmIpv6Mask_0_64_Free, *stats.lpm_ipv6_mask_0_64_free_ref());
  fb303::fbData->setCounter(
      kLpmIpv6Mask_65_127_Max, *stats.lpm_ipv6_mask_65_127_max_ref());
  fb303::fbData->setCounter(
      kLpmIpv6Mask_65_127_Used, *stats.lpm_ipv6_mask_65_127_used_ref());
  fb303::fbData->setCounter(
      kLpmIpv6Mask_65_127_Free, *stats.lpm_ipv6_mask_65_127_free_ref());
  fb303::fbData->setCounter(kLpmTableMax, *stats.lpm_slots_max_ref());
  fb303::fbData->setCounter(kLpmTableUsed, *stats.lpm_slots_used_ref());
  fb303::fbData->setCounter(kLpmTableFree, *stats.lpm_slots_free_ref());
  // Acl
  fb303::fbData->setCounter(kAclEntriesMax, *stats.acl_entries_max_ref());
  fb303::fbData->setCounter(kAclEntriesUsed, *stats.acl_entries_used_ref());
  fb303::fbData->setCounter(kAclEntriesFree, *stats.acl_entries_free_ref());
  fb303::fbData->setCounter(kAclCountersMax, *stats.acl_counters_max_ref());
  fb303::fbData->setCounter(kAclCountersUsed, *stats.acl_counters_used_ref());
  fb303::fbData->setCounter(kAclCountersFree, *stats.acl_counters_free_ref());
  fb303::fbData->setCounter(kAclMetersMax, *stats.acl_meters_max_ref());
  fb303::fbData->setCounter(kAclMetersUsed, *stats.acl_meters_used_ref());
  fb303::fbData->setCounter(kAclMetersFree, *stats.acl_meters_free_ref());
  // Mirrors
  fb303::fbData->setCounter(kMirrorsMax, *stats.mirrors_max_ref());
  fb303::fbData->setCounter(kMirrorsUsed, *stats.mirrors_used_ref());
  fb303::fbData->setCounter(kMirrorsFree, *stats.mirrors_free_ref());
  fb303::fbData->setCounter(kMirrorsSpan, *stats.mirrors_span_ref());
  fb303::fbData->setCounter(kMirrorsErspan, *stats.mirrors_erspan_ref());
  fb303::fbData->setCounter(kMirrorsSflow, *stats.mirrors_sflow_ref());
}

} // namespace facebook::fboss
