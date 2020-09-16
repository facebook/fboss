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

void HwResourceStatsPublisher::publish(folly::StringPiece name, int64_t value)
    const {
  if (value == hardware_stats_constants::STAT_UNINITIALIZED()) {
    return;
  }
  fb303::fbData->setCounter(name, value);
}

void HwResourceStatsPublisher::publish(const HwResourceStats& stats) const {
  publish(kHwTableStatsStale, *stats.hw_table_stats_stale_ref() ? 1 : 0);
  publish(kL3HostMax, *stats.l3_host_max_ref());
  publish(kL3HostUsed, *stats.l3_host_used_ref());
  publish(kL3HostFree, *stats.l3_host_free_ref());
  publish(kL3Ipv4HostUsed, *stats.l3_ipv4_host_used_ref());
  publish(kL3Ipv4HostFree, *stats.l3_ipv4_host_free_ref());
  publish(kL3Ipv6HostUsed, *stats.l3_ipv6_host_used_ref());
  publish(kL3Ipv6HostFree, *stats.l3_ipv6_host_free_ref());
  publish(kL3NextHopsMax, *stats.l3_nexthops_max_ref());
  publish(kL3NextHopsUsed, *stats.l3_nexthops_used_ref());
  publish(kL3NextHopsFree, *stats.l3_nexthops_free_ref());
  publish(kL3Ipv4NextHopsFree, *stats.l3_ipv4_nexthops_free_ref());
  publish(kL3Ipv6NextHopsFree, *stats.l3_ipv6_nexthops_free_ref());
  publish(kL3EcmpGroupsMax, *stats.l3_ecmp_groups_max_ref());
  publish(kL3EcmpGroupsUsed, *stats.l3_ecmp_groups_used_ref());
  publish(kL3EcmpGroupsFree, *stats.l3_ecmp_groups_free_ref());
  publish(kL3EcmpGroupMembersFree, *stats.l3_ecmp_group_members_free_ref());

  // LPM
  publish(kLpmIpv4Max, *stats.lpm_ipv4_max_ref());
  publish(kLpmIpv4Used, *stats.lpm_ipv4_used_ref());
  publish(kLpmIpv4Free, *stats.lpm_ipv4_free_ref());
  publish(kLpmIpv6Mask_0_64_Max, *stats.lpm_ipv6_mask_0_64_max_ref());
  publish(kLpmIpv6Mask_0_64_Used, *stats.lpm_ipv6_mask_0_64_used_ref());
  publish(kLpmIpv6Mask_0_64_Free, *stats.lpm_ipv6_mask_0_64_free_ref());
  publish(kLpmIpv6Mask_65_127_Max, *stats.lpm_ipv6_mask_65_127_max_ref());
  publish(kLpmIpv6Mask_65_127_Used, *stats.lpm_ipv6_mask_65_127_used_ref());
  publish(kLpmIpv6Mask_65_127_Free, *stats.lpm_ipv6_mask_65_127_free_ref());
  publish(kLpmIpv6Free, *stats.lpm_ipv6_free_ref());
  publish(kLpmTableMax, *stats.lpm_slots_max_ref());
  publish(kLpmTableUsed, *stats.lpm_slots_used_ref());
  publish(kLpmTableFree, *stats.lpm_slots_free_ref());
  // Acl
  publish(kAclEntriesMax, *stats.acl_entries_max_ref());
  publish(kAclEntriesUsed, *stats.acl_entries_used_ref());
  publish(kAclEntriesFree, *stats.acl_entries_free_ref());
  publish(kAclCountersMax, *stats.acl_counters_max_ref());
  publish(kAclCountersUsed, *stats.acl_counters_used_ref());
  publish(kAclCountersFree, *stats.acl_counters_free_ref());
  publish(kAclMetersMax, *stats.acl_meters_max_ref());
  publish(kAclMetersUsed, *stats.acl_meters_used_ref());
  publish(kAclMetersFree, *stats.acl_meters_free_ref());
  // Mirrors
  publish(kMirrorsMax, *stats.mirrors_max_ref());
  publish(kMirrorsUsed, *stats.mirrors_used_ref());
  publish(kMirrorsFree, *stats.mirrors_free_ref());
  publish(kMirrorsSpan, *stats.mirrors_span_ref());
  publish(kMirrorsErspan, *stats.mirrors_erspan_ref());
  publish(kMirrorsSflow, *stats.mirrors_sflow_ref());
}

} // namespace facebook::fboss
