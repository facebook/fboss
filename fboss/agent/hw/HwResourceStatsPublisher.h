/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/Range.h>

namespace facebook::fboss {

struct HwResourceStats;

constexpr folly::StringPiece kHwTableStatsStale{"hw_table_stats_stale"};
constexpr folly::StringPiece kL3HostMax{"l3_host_max"};
constexpr folly::StringPiece kL3HostUsed{"l3_host_used"};
constexpr folly::StringPiece kL3HostFree{"l3_host_free"};
constexpr folly::StringPiece kL3Ipv4HostUsed{"l3_ipv4_host_used"};
constexpr folly::StringPiece kL3Ipv4HostFree{"l3_ipv4_host_free"};
constexpr folly::StringPiece kL3Ipv6HostUsed{"l3_ipv6_host_used"};
constexpr folly::StringPiece kL3Ipv6HostFree{"l3_ipv6_host_free"};
constexpr folly::StringPiece kL3NextHopsMax{"l3_nexthops_max"};
constexpr folly::StringPiece kL3NextHopsUsed{"l3_nexthops_used"};
constexpr folly::StringPiece kL3NextHopsFree{"l3_nexthops_free"};
constexpr folly::StringPiece kL3Ipv4NextHopsFree{"l3_ipv4_nexthops_free"};
constexpr folly::StringPiece kL3Ipv6NextHopsFree{"l3_ipv6_nexthops_free"};
constexpr folly::StringPiece kL3EcmpGroupsMax{"l3_ecmp_groups_max"};
constexpr folly::StringPiece kL3EcmpGroupsUsed{"l3_ecmp_groups_used"};
constexpr folly::StringPiece kL3EcmpGroupsFree{"l3_ecmp_groups_free"};
constexpr folly::StringPiece kL3EcmpGroupMembersFree{
    "l3_ecmp_group_memers_free"};
constexpr folly::StringPiece kLpmIpv4Max{"lpm_ipv4_max"};
constexpr folly::StringPiece kLpmIpv4Used{"lpm_ipv4_used"};
constexpr folly::StringPiece kLpmIpv4Free{"lpm_ipv4_free"};
constexpr folly::StringPiece kLpmIpv6Mask_0_64_Max{"lpm_ipv6_mask_0_64_max"};
constexpr folly::StringPiece kLpmIpv6Mask_0_64_Used{"lpm_ipv6_mask_0_64_used"};
constexpr folly::StringPiece kLpmIpv6Mask_0_64_Free{"lpm_ipv6_mask_0_64_free"};
constexpr folly::StringPiece kLpmIpv6Mask_65_127_Max{
    "lpm_ipv6_mask_65_127_max"};
constexpr folly::StringPiece kLpmIpv6Mask_65_127_Used{
    "lpm_ipv6_mask_65_127_used"};
constexpr folly::StringPiece kLpmIpv6Mask_65_127_Free{
    "lpm_ipv6_mask_65_127_free"};
constexpr folly::StringPiece kLpmIpv6Free{"lpm_ipv6_free"};
constexpr folly::StringPiece kLpmTableMax{"lpm_table_max"};
constexpr folly::StringPiece kLpmTableUsed{"lpm_table_used"};
constexpr folly::StringPiece kLpmTableFree{"lpm_table_free"};
constexpr folly::StringPiece kAclEntriesMax{"acl_entries_max"};
constexpr folly::StringPiece kAclEntriesUsed{"acl_entries_used"};
constexpr folly::StringPiece kAclEntriesFree{"acl_entries_free"};
constexpr folly::StringPiece kAclCountersMax{"acl_counters_max"};
constexpr folly::StringPiece kAclCountersUsed{"acl_counters_used"};
constexpr folly::StringPiece kAclCountersFree{"acl_counters_free"};
constexpr folly::StringPiece kAclMetersMax{"acl_meters_max"};
constexpr folly::StringPiece kAclMetersUsed{"acl_meters_used"};
constexpr folly::StringPiece kAclMetersFree{"acl_meters_free"};
constexpr folly::StringPiece kMirrorsMax{"mirrors_max"};
constexpr folly::StringPiece kMirrorsUsed{"mirrors_used"};
constexpr folly::StringPiece kMirrorsFree{"mirrors_free"};
constexpr folly::StringPiece kMirrorsSpan{"mirrors_span"};
constexpr folly::StringPiece kMirrorsErspan{"mirrors_erspan"};
constexpr folly::StringPiece kMirrorsSflow{"mirrors_sflow"};

class HwResourceStatsPublisher {
 public:
  void publish(const HwResourceStats& stats) const;

 private:
  void publish(folly::StringPiece name, int64_t value) const;
};

} // namespace facebook::fboss
