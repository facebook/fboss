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
  fb303::fbData->setCounter(
      statsPrefix_ ? fmt::format("{}{}", *statsPrefix_, name) : name, value);
}

void HwResourceStatsPublisher::publish(const HwResourceStats& stats) const {
  publish(kHwTableStatsStale, *stats.hw_table_stats_stale() ? 1 : 0);
  publish(kL3HostMax, *stats.l3_host_max());
  publish(kL3HostUsed, *stats.l3_host_used());
  publish(kL3HostFree, *stats.l3_host_free());
  publish(kL3Ipv4HostUsed, *stats.l3_ipv4_host_used());
  publish(kL3Ipv4HostFree, *stats.l3_ipv4_host_free());
  publish(kL3Ipv6HostUsed, *stats.l3_ipv6_host_used());
  publish(kL3Ipv6HostFree, *stats.l3_ipv6_host_free());
  publish(kL3NextHopsMax, *stats.l3_nexthops_max());
  publish(kL3NextHopsUsed, *stats.l3_nexthops_used());
  publish(kL3NextHopsFree, *stats.l3_nexthops_free());
  publish(kL3Ipv4NextHopsFree, *stats.l3_ipv4_nexthops_free());
  publish(kL3Ipv6NextHopsFree, *stats.l3_ipv6_nexthops_free());
  publish(kL3EcmpGroupsMax, *stats.l3_ecmp_groups_max());
  publish(kL3EcmpGroupsUsed, *stats.l3_ecmp_groups_used());
  publish(kL3EcmpGroupsFree, *stats.l3_ecmp_groups_free());
  publish(kL3EcmpGroupMembersFree, *stats.l3_ecmp_group_members_free());

  // LPM
  publish(kLpmIpv4Max, *stats.lpm_ipv4_max());
  publish(kLpmIpv4Used, *stats.lpm_ipv4_used());
  publish(kLpmIpv4Free, *stats.lpm_ipv4_free());
  publish(kLpmIpv6Mask_0_64_Max, *stats.lpm_ipv6_mask_0_64_max());
  publish(kLpmIpv6Mask_0_64_Used, *stats.lpm_ipv6_mask_0_64_used());
  publish(kLpmIpv6Mask_0_64_Free, *stats.lpm_ipv6_mask_0_64_free());
  publish(kLpmIpv6Mask_65_127_Max, *stats.lpm_ipv6_mask_65_127_max());
  publish(kLpmIpv6Mask_65_127_Used, *stats.lpm_ipv6_mask_65_127_used());
  publish(kLpmIpv6Mask_65_127_Free, *stats.lpm_ipv6_mask_65_127_free());
  publish(kLpmIpv6Free, *stats.lpm_ipv6_free());
  publish(kLpmTableMax, *stats.lpm_slots_max());
  publish(kLpmTableUsed, *stats.lpm_slots_used());
  publish(kLpmTableFree, *stats.lpm_slots_free());
  // Acl
  publish(kAclEntriesMax, *stats.acl_entries_max());
  publish(kAclEntriesUsed, *stats.acl_entries_used());
  publish(kAclEntriesFree, *stats.acl_entries_free());
  publish(kAclCountersMax, *stats.acl_counters_max());
  publish(kAclCountersUsed, *stats.acl_counters_used());
  publish(kAclCountersFree, *stats.acl_counters_free());
  publish(kAclMetersMax, *stats.acl_meters_max());
  publish(kAclMetersUsed, *stats.acl_meters_used());
  publish(kAclMetersFree, *stats.acl_meters_free());
  // Mirrors
  publish(kMirrorsMax, *stats.mirrors_max());
  publish(kMirrorsUsed, *stats.mirrors_used());
  publish(kMirrorsFree, *stats.mirrors_free());
  publish(kMirrorsSpan, *stats.mirrors_span());
  publish(kMirrorsErspan, *stats.mirrors_erspan());
  publish(kMirrorsSflow, *stats.mirrors_sflow());
  // EM
  publish(kEmEntriesMax, *stats.em_entries_max());
  publish(kEmEntriesUsed, *stats.em_entries_used());
  publish(kEmEntriesFree, *stats.em_entries_free());
  publish(kEmCountersMax, *stats.em_counters_max());
  publish(kEmCountersUsed, *stats.em_counters_used());
  publish(kEmCountersFree, *stats.em_counters_free());
  // VOQs
  publish(kSystemPortsFree, *stats.system_ports_free());
  publish(kVoqsFree, *stats.voqs_free());
}

} // namespace facebook::fboss
