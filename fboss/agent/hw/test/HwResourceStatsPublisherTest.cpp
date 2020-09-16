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
constexpr std::array<folly::StringPiece, 39> kAllStatKeys = {
    kHwTableStatsStale,
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
    kMirrorsSflow};
}

void checkMissing(const std::set<folly::StringPiece>& present) {
  for (auto name : kAllStatKeys) {
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
