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

#include <gtest/gtest.h>

using namespace facebook::fboss;
using namespace facebook::fb303;

namespace {
constexpr std::array<folly::StringPiece, 44> kAllMonotonicCounterStatKeys = {
    kL3HostMax,
    kL3HostUsed,
    kL3HostFree,
    kL3NextHopsMax,
    kL3NextHopsUsed,
    kL3NextHopsFree,
    kL3EcmpGroupsMax,
    kL3EcmpGroupsUsed,
    kL3EcmpGroupsFree,
    kL3Ipv4HostUsed,
    kL3Ipv6HostUsed,
    kLpmIpv4Max,
    kLpmIpv4Used,
    kLpmIpv4Free,
    kLpmIpv6Mask_0_64_Max,
    kLpmIpv6Mask_0_64_Used,
    kLpmIpv6Mask_0_64_Free,
    kLpmIpv6Mask_65_127_Max,
    kLpmIpv6Mask_65_127_Used,
    kLpmIpv6Mask_65_127_Free,
    kLpmTableMax,
    kLpmTableUsed,
    kLpmTableFree,
    kAclEntriesMax,
    kAclEntriesUsed,
    kAclEntriesFree,
    kAclCountersMax,
    kAclCountersUsed,
    kAclCountersFree,
    kAclMetersMax,
    kAclMetersUsed,
    kAclMetersFree,
    kMirrorsMax,
    kMirrorsUsed,
    kMirrorsFree,
    kMirrorsSpan,
    kMirrorsErspan,
    kMirrorsSflow,
    kEmEntriesMax,
    kEmEntriesUsed,
    kEmEntriesFree,
    kEmCountersMax,
    kEmCountersUsed,
    kEmCountersFree};
}

void checkMissing(const std::set<folly::StringPiece>& present) {
  for (auto name : kAllMonotonicCounterStatKeys) {
    if (present.find(name) == present.end()) {
      EXPECT_THROW(fbData->getCounter(name), std::invalid_argument);
    } else {
      EXPECT_NE(
          fbData->getCounter(name),
          hardware_stats_constants::STAT_UNINITIALIZED());
    }
  }
}

TEST(HwResourceStatsPublisherTest, StatsNotInitialized) {
  checkMissing({});
}

TEST(HwResourceStatsPublisherTest, StatsStale) {
  HwResourceStats stats;
  stats.hw_table_stats_stale() = true;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kHwTableStatsStale), 1);
  checkMissing({});
}

TEST(HwResourceStatsPublisher, AclStats) {
  HwResourceStats stats;
  stats.acl_entries_max() = 10;
  stats.acl_entries_used() = 1;
  stats.acl_entries_free() = 9;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kAclEntriesMax), 10);
  EXPECT_EQ(fbData->getCounter(kAclEntriesUsed), 1);
  EXPECT_EQ(fbData->getCounter(kAclEntriesFree), 9);
  checkMissing({kAclEntriesMax, kAclEntriesUsed, kAclEntriesFree});
}

TEST(HwResourceStatsPublisher, AclCounterStats) {
  HwResourceStats stats;
  stats.acl_counters_max() = 10;
  stats.acl_counters_used() = 1;
  stats.acl_counters_free() = 9;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kAclCountersMax), 10);
  EXPECT_EQ(fbData->getCounter(kAclCountersUsed), 1);
  EXPECT_EQ(fbData->getCounter(kAclCountersFree), 9);
  checkMissing({kAclCountersMax, kAclCountersUsed, kAclCountersFree});
}

TEST(HwResourceStatsPublisher, AclMeterStats) {
  HwResourceStats stats;
  stats.acl_meters_max() = 10;
  stats.acl_meters_used() = 1;
  stats.acl_meters_free() = 9;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kAclMetersMax), 10);
  EXPECT_EQ(fbData->getCounter(kAclMetersUsed), 1);
  EXPECT_EQ(fbData->getCounter(kAclMetersFree), 9);
  checkMissing({kAclMetersMax, kAclMetersUsed, kAclMetersFree});
}

TEST(HwResourceStatsPublisher, MirrorStats) {
  HwResourceStats stats;
  stats.mirrors_max() = 10;
  stats.mirrors_used() = 6;
  stats.mirrors_free() = 4;
  stats.mirrors_span() = 1;
  stats.mirrors_erspan() = 2;
  stats.mirrors_sflow() = 3;

  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kMirrorsMax), 10);
  EXPECT_EQ(fbData->getCounter(kMirrorsUsed), 6);
  EXPECT_EQ(fbData->getCounter(kMirrorsFree), 4);
  EXPECT_EQ(fbData->getCounter(kMirrorsSpan), 1);
  EXPECT_EQ(fbData->getCounter(kMirrorsErspan), 2);
  EXPECT_EQ(fbData->getCounter(kMirrorsSflow), 3);
  checkMissing(
      {kMirrorsMax,
       kMirrorsUsed,
       kMirrorsFree,
       kMirrorsSpan,
       kMirrorsErspan,
       kMirrorsSflow});
}

TEST(HwResourceStatsPublisher, NextHopStats) {
  HwResourceStats stats;
  stats.l3_nexthops_max() = 10;
  stats.l3_nexthops_used() = 1;
  stats.l3_nexthops_free() = 9;
  stats.l3_ipv4_nexthops_free() = 8;
  stats.l3_ipv6_nexthops_free() = 7;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kL3NextHopsMax), 10);
  EXPECT_EQ(fbData->getCounter(kL3NextHopsUsed), 1);
  EXPECT_EQ(fbData->getCounter(kL3NextHopsFree), 9);
  EXPECT_EQ(fbData->getCounter(kL3Ipv4NextHopsFree), 8);
  EXPECT_EQ(fbData->getCounter(kL3Ipv6NextHopsFree), 7);
  checkMissing({kL3NextHopsMax, kL3NextHopsUsed, kL3NextHopsFree});
}

TEST(HwResourceStatsPublisher, EcmpGroupStats) {
  HwResourceStats stats;
  stats.l3_ecmp_groups_max() = 10;
  stats.l3_ecmp_groups_used() = 1;
  stats.l3_ecmp_groups_free() = 9;
  stats.l3_ecmp_group_members_free() = 42;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kL3EcmpGroupsMax), 10);
  EXPECT_EQ(fbData->getCounter(kL3EcmpGroupsUsed), 1);
  EXPECT_EQ(fbData->getCounter(kL3EcmpGroupsFree), 9);
  EXPECT_EQ(fbData->getCounter(kL3EcmpGroupMembersFree), 42);
  checkMissing(
      {kL3EcmpGroupsMax,
       kL3EcmpGroupsUsed,
       kL3EcmpGroupsFree,
       kL3EcmpGroupMembersFree});
}

TEST(HwResourceStatsPublisher, HostStats) {
  HwResourceStats stats;
  stats.l3_host_max() = 10;
  stats.l3_host_used() = 4;
  stats.l3_host_free() = 6;
  stats.l3_ipv4_host_used() = 4;
  stats.l3_ipv4_host_free() = 2;
  stats.l3_ipv6_host_used() = 5;
  stats.l3_ipv6_host_free() = 3;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kL3HostMax), 10);
  EXPECT_EQ(fbData->getCounter(kL3HostUsed), 4);
  EXPECT_EQ(fbData->getCounter(kL3HostFree), 6);
  EXPECT_EQ(fbData->getCounter(kL3Ipv4HostUsed), 4);
  EXPECT_EQ(fbData->getCounter(kL3Ipv4HostFree), 2);
  EXPECT_EQ(fbData->getCounter(kL3Ipv6HostUsed), 5);
  EXPECT_EQ(fbData->getCounter(kL3Ipv6HostFree), 3);
  checkMissing({
      kL3HostMax,
      kL3HostUsed,
      kL3HostFree,
      kL3Ipv4HostUsed,
      kL3Ipv4HostFree,
      kL3Ipv6HostUsed,
      kL3Ipv6HostFree,
  });
}

TEST(HwResourceStatsPublisher, Ipv4RouteStats) {
  HwResourceStats stats;
  stats.lpm_ipv4_max() = 10;
  stats.lpm_ipv4_used() = 1;
  stats.lpm_ipv4_free() = 9;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kLpmIpv4Max), 10);
  EXPECT_EQ(fbData->getCounter(kLpmIpv4Used), 1);
  EXPECT_EQ(fbData->getCounter(kLpmIpv4Free), 9);
  checkMissing({kLpmIpv4Max, kLpmIpv4Used, kLpmIpv4Free});
}

TEST(HwResourceStatsPublisher, Ipv6RouteStats) {
  HwResourceStats stats;
  stats.lpm_ipv6_mask_0_64_max() = 10;
  stats.lpm_ipv6_mask_0_64_used() = 1;
  stats.lpm_ipv6_mask_0_64_free() = 9;
  stats.lpm_ipv6_mask_65_127_max() = 5;
  stats.lpm_ipv6_mask_65_127_used() = 2;
  stats.lpm_ipv6_mask_65_127_free() = 3;
  stats.lpm_ipv6_free() = 12;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kLpmIpv6Mask_0_64_Max), 10);
  EXPECT_EQ(fbData->getCounter(kLpmIpv6Mask_0_64_Used), 1);
  EXPECT_EQ(fbData->getCounter(kLpmIpv6Mask_0_64_Free), 9);
  EXPECT_EQ(fbData->getCounter(kLpmIpv6Mask_65_127_Max), 5);
  EXPECT_EQ(fbData->getCounter(kLpmIpv6Mask_65_127_Used), 2);
  EXPECT_EQ(fbData->getCounter(kLpmIpv6Mask_65_127_Free), 3);
  EXPECT_EQ(fbData->getCounter(kLpmIpv6Free), 12);
  checkMissing(
      {kLpmIpv6Mask_0_64_Max,
       kLpmIpv6Mask_0_64_Used,
       kLpmIpv6Mask_0_64_Free,
       kLpmIpv6Mask_65_127_Max,
       kLpmIpv6Mask_65_127_Used,
       kLpmIpv6Mask_65_127_Free,
       kLpmIpv6Free});
}

TEST(HwResourceStatsPublisher, LpmSlotStats) {
  HwResourceStats stats;
  stats.lpm_slots_max() = 10;
  stats.lpm_slots_used() = 1;
  stats.lpm_slots_free() = 9;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kLpmTableMax), 10);
  EXPECT_EQ(fbData->getCounter(kLpmTableUsed), 1);
  EXPECT_EQ(fbData->getCounter(kLpmTableFree), 9);
  checkMissing({kLpmTableMax, kLpmTableUsed, kLpmTableFree});
}

TEST(HwResourceStatsPublisher, EmStats) {
  HwResourceStats stats;
  stats.em_entries_max() = 10;
  stats.em_entries_used() = 1;
  stats.em_entries_free() = 9;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kEmEntriesMax), 10);
  EXPECT_EQ(fbData->getCounter(kEmEntriesUsed), 1);
  EXPECT_EQ(fbData->getCounter(kEmEntriesFree), 9);
  checkMissing({kEmEntriesMax, kEmEntriesUsed, kEmEntriesFree});
}

TEST(HwResourceStatsPublisher, EmCounterStats) {
  HwResourceStats stats;
  stats.em_counters_max() = 10;
  stats.em_counters_used() = 1;
  stats.em_counters_free() = 9;
  HwResourceStatsPublisher().publish(stats);
  EXPECT_EQ(fbData->getCounter(kEmCountersMax), 10);
  EXPECT_EQ(fbData->getCounter(kEmCountersUsed), 1);
  EXPECT_EQ(fbData->getCounter(kEmCountersFree), 9);
  checkMissing({kEmCountersMax, kEmCountersUsed, kEmCountersFree});
}
