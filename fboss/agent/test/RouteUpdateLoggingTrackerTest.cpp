/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/IPAddress.h>
#include "fboss/agent/RouteUpdateLoggingPrefixTracker.h"
#include "fboss/agent/state/RouteTypes.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

namespace {
// Test some plausible sequences of tracking, checking, and stopping tracking

class PrefixTrackerTest : public ::testing::Test {
 public:
  void startTracking(
      const std::string& addr,
      uint8_t mask,
      const std::string& identifier,
      bool exact) {
    RoutePrefix<folly::IPAddress> prefix{folly::IPAddress{addr}, mask};
    auto req =
        std::make_unique<RouteUpdateLoggingInstance>(prefix, identifier, exact);
    tracker.track(*req);
  }
  void stopTracking(
      const std::string& addr,
      uint8_t mask,
      const std::string& identifier) {
    RoutePrefix<folly::IPAddress> prefix{folly::IPAddress{addr}, mask};
    tracker.stopTracking(prefix, identifier);
  }

  template <typename PrefixT>
  void checkTracking(const PrefixT& prefix) {
    std::vector<std::string> ids;
    EXPECT_TRUE(tracker.tracking(prefix, ids));
  }

  template <typename PrefixT>
  void checkNotTracking(const PrefixT& prefix) {
    std::vector<std::string> ids;
    EXPECT_FALSE(tracker.tracking(prefix, ids));
  }

  RouteUpdateLoggingPrefixTracker tracker;
  RoutePrefix<folly::IPAddressV6> p1{folly::IPAddressV6{"1:1:1:1::"}, 64};
  RoutePrefix<folly::IPAddressV6> p2{folly::IPAddressV6{"1:1::"}, 32};
  RoutePrefix<folly::IPAddressV6> pFull{folly::IPAddressV6{"::1:1:1:1"}, 128};
  RoutePrefix<folly::IPAddressV6> pZero{folly::IPAddressV6{"::"}, 0};
};

// Track any prefix. Check a couple arbitrary ones
TEST_F(PrefixTrackerTest, TrackAny) {
  checkNotTracking(p1);
  checkNotTracking(pFull);
  checkNotTracking(pZero);
  startTracking("::", 0, "", false);
  checkTracking(p1);
  checkTracking(pFull);
  checkTracking(pZero);
}

// Checking a prefix shouldn't affect future checks
TEST_F(PrefixTrackerTest, IdempotentTrackingCheck) {
  checkNotTracking(p1);
  checkNotTracking(p1);
}

// We don't implicitly track the all zeros address
TEST_F(PrefixTrackerTest, AllZeroUntracked) {
  RoutePrefix<folly::IPAddressV6> z{folly::IPAddressV6{"1::1"}, 0};
  checkNotTracking(pZero);
  checkNotTracking(z);
}

// Exact network/mask match
TEST_F(PrefixTrackerTest, ExactMatch) {
  checkNotTracking(p1);
  startTracking("1:1:1:1::", 64, "", false);
  checkTracking(p1);
}

// Track a less specific prefix than what we check
TEST_F(PrefixTrackerTest, PartialMatch) {
  checkNotTracking(p1);
  startTracking("1:1::", 32, "", false);
  checkTracking(p1);
}

// Track a less specific prefix than what we check in exact mode
TEST_F(PrefixTrackerTest, PartialMatchTrackExact) {
  checkNotTracking(p1);
  startTracking("1:1::", 32, "", true);
  checkNotTracking(p1);
}

// Track a more specific prefix than what we check
TEST_F(PrefixTrackerTest, TrackedTooSpecific) {
  checkNotTracking(p1);
  startTracking("1:1:2:2::", 64, "", false);
  checkNotTracking(p1);
}

// Stop tracking a tracked prefix.
TEST_F(PrefixTrackerTest, DeleteTracked) {
  checkNotTracking(p1);
  startTracking("::", 0, "", false);
  checkTracking(p1);
  stopTracking("::", 0, "");
  checkNotTracking(p1);
}

// Try to stop tracking a prefix that we weren't tracking
TEST_F(PrefixTrackerTest, DeleteUntracked) {
  checkNotTracking(p1);
  startTracking("1:1::", 32, "", false);
  checkTracking(p1);
  stopTracking("1:1:1:1::", 64, "");
  checkTracking(p1);
}

// Track two prefixes with one being more specific than the other.
// Stop tracking the less specific prefix, but the more specific prefix
// should still be tracked
TEST_F(PrefixTrackerTest, DeleteLessSpecificTracked) {
  checkNotTracking(p1);
  checkNotTracking(p2);
  startTracking("1:1::", 32, "", false);
  startTracking("1:1:1:1::", 64, "", false);
  checkTracking(p1);
  checkTracking(p2);
  stopTracking("1:1::", 32, "");
  checkTracking(p1);
  checkNotTracking(p2);
}

// Try to stop tracking a less specific (but untracked) prefix than one we are
// actually tracking. The more specific prefix should still be tracked
TEST_F(PrefixTrackerTest, DeleteLessSpecificUntracked) {
  checkNotTracking(p1);
  checkNotTracking(p2);
  startTracking("1:1:1:1::", 64, "", false);
  checkTracking(p1);
  checkNotTracking(p2);
  stopTracking("1:1::", 32, "");
  checkTracking(p1);
  checkNotTracking(p2);
}

} // namespace
