// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/PfcUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/state/PortPgConfig.h"
#include "fboss/agent/types.h"

using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;

namespace facebook::fboss {
namespace {

// A PG is lossless iff it has non-zero headroom. Note: this relies on
// FLAGS_allow_zero_headroom_for_lossless_pg defaulting to false (see
// utility::isLosslessPg); a non-zero headroom makes a PG lossless regardless.
std::shared_ptr<PortPgConfig> makePg(uint8_t id, bool lossless) {
  auto pg = std::make_shared<PortPgConfig>(id);
  if (lossless) {
    pg->setHeadroomLimitBytes(1000);
  }
  return pg;
}

// Unwrap a vector<PfcPriority> to the underlying ints for easy assertions.
std::vector<int> unwrap(const std::vector<PfcPriority>& priorities) {
  std::vector<int> out;
  out.reserve(priorities.size());
  for (auto p : priorities) {
    out.push_back(static_cast<int>(p));
  }
  return out;
}

} // namespace

// ---- findPfcEnabledPgIds: lossless PGs, independent of any map ----

// Unsorted input still yields lossless PG ids in ascending order.
TEST(PfcUtilsTest, findPfcEnabledPgIdsReturnsLosslessPgsAscending) {
  PortPgConfigs pgs{
      makePg(2, true), makePg(1, false), makePg(0, true), makePg(3, false)};
  EXPECT_THAT(utility::findPfcEnabledPgIds(pgs), ElementsAre(0, 2));
}

TEST(PfcUtilsTest, findPfcEnabledPgIdsEmptyWhenNoLossless) {
  PortPgConfigs pgs{makePg(0, false), makePg(1, false)};
  EXPECT_TRUE(utility::findPfcEnabledPgIds(pgs).empty());
}

TEST(PfcUtilsTest, findPfcEnabledPgIdsEmptyWhenNoPgs) {
  PortPgConfigs pgs{};
  EXPECT_TRUE(utility::findPfcEnabledPgIds(pgs).empty());
}

// ---- findPfcEnabledPriorities ----

// Empty map => identity fallback: priorities == lossless PG ids in ascending
// order (today's behavior, since the old findEnabledPfcPriorities pushed PG ids
// directly). Ordered assertion pins the backward-compat no-op invariant.
TEST(PfcUtilsTest, findPfcEnabledPrioritiesIdentityFallbackWhenMapEmpty) {
  PortPgConfigs pgs{makePg(3, true), makePg(2, true), makePg(4, false)};
  std::map<int16_t, int16_t> emptyMap;
  EXPECT_THAT(
      unwrap(utility::findPfcEnabledPriorities(pgs, emptyMap)),
      ElementsAre(2, 3));
}

// Identity fallback with no lossless PG => empty (legacy nullopt case b).
TEST(PfcUtilsTest, findPfcEnabledPrioritiesIdentityEmptyWhenNoLosslessPg) {
  PortPgConfigs pgs{makePg(0, false), makePg(1, false)};
  std::map<int16_t, int16_t> emptyMap;
  EXPECT_TRUE(utility::findPfcEnabledPriorities(pgs, emptyMap).empty());
}

// Identity fallback with no PGs at all => empty (legacy nullopt case a).
TEST(PfcUtilsTest, findPfcEnabledPrioritiesIdentityEmptyWhenNoPgs) {
  PortPgConfigs pgs{};
  std::map<int16_t, int16_t> emptyMap;
  EXPECT_TRUE(utility::findPfcEnabledPriorities(pgs, emptyMap).empty());
}

// Non-identity map but no PG is lossless => no priorities enabled.
TEST(PfcUtilsTest, findPfcEnabledPrioritiesNonIdentityEmptyWhenNoLosslessPg) {
  PortPgConfigs pgs{makePg(0, false), makePg(1, false)};
  std::map<int16_t, int16_t> pfcPriToPg{{3, 0}, {4, 1}};
  EXPECT_TRUE(utility::findPfcEnabledPriorities(pgs, pfcPriToPg).empty());
}

// Non-identity map: only the priority whose mapped PG is lossless is enabled.
TEST(PfcUtilsTest, findPfcEnabledPrioritiesNonIdentityMap) {
  PortPgConfigs pgs{makePg(0, true), makePg(1, false)};
  // pri 3 -> PG0 (lossless), pri 4 -> PG1 (lossy)
  std::map<int16_t, int16_t> pfcPriToPg{{3, 0}, {4, 1}};
  EXPECT_THAT(
      unwrap(utility::findPfcEnabledPriorities(pgs, pfcPriToPg)),
      UnorderedElementsAre(3));
}

// A priority mapping to a non-existent PG id contributes nothing.
TEST(PfcUtilsTest, findPfcEnabledPrioritiesUnknownPgIgnored) {
  PortPgConfigs pgs{makePg(0, true)};
  std::map<int16_t, int16_t> pfcPriToPg{{3, 7}}; // PG7 doesn't exist
  EXPECT_TRUE(utility::findPfcEnabledPriorities(pgs, pfcPriToPg).empty());
}

// ---- findPfcEnabledQueues ----

// Empty map => identity fallback: queue == priority.
TEST(PfcUtilsTest, findPfcEnabledQueuesIdentityFallbackWhenMapEmpty) {
  std::vector<PfcPriority> priorities{PfcPriority(3), PfcPriority(4)};
  std::map<int16_t, int16_t> emptyMap;
  EXPECT_THAT(
      utility::findPfcEnabledQueues(priorities, emptyMap),
      UnorderedElementsAre(3, 4));
}

// Non-identity map: priorities resolve to their mapped queues, ascending.
TEST(PfcUtilsTest, findPfcEnabledQueuesNonIdentityMap) {
  std::vector<PfcPriority> priorities{PfcPriority(3), PfcPriority(4)};
  std::map<int16_t, int16_t> pfcPriToQueue{{3, 7}, {4, 2}};
  EXPECT_THAT(
      utility::findPfcEnabledQueues(priorities, pfcPriToQueue),
      ElementsAre(2, 7));
}

// A non-empty queue map is the source of truth: an enabled priority that is
// absent from the map contributes no queue (it is skipped, not
// identity-mapped).
TEST(PfcUtilsTest, findPfcEnabledQueuesSkipsPriorityAbsentFromNonEmptyMap) {
  std::vector<PfcPriority> priorities{PfcPriority(3), PfcPriority(4)};
  std::map<int16_t, int16_t> pfcPriToQueue{{3, 7}}; // pri 4 not mapped
  EXPECT_THAT(
      utility::findPfcEnabledQueues(priorities, pfcPriToQueue), ElementsAre(7));
}

// Distinct priorities mapping to the same queue collapse to one queue.
TEST(PfcUtilsTest, findPfcEnabledQueuesDedupsSharedQueue) {
  std::vector<PfcPriority> priorities{PfcPriority(3), PfcPriority(4)};
  std::map<int16_t, int16_t> pfcPriToQueue{{3, 1}, {4, 1}};
  EXPECT_THAT(
      utility::findPfcEnabledQueues(priorities, pfcPriToQueue),
      UnorderedElementsAre(1));
}

TEST(PfcUtilsTest, findPfcEnabledQueuesEmptyPrioritiesEmptyResult) {
  std::vector<PfcPriority> priorities{};
  std::map<int16_t, int16_t> pfcPriToQueue{{3, 7}};
  EXPECT_TRUE(utility::findPfcEnabledQueues(priorities, pfcPriToQueue).empty());
}

} // namespace facebook::fboss
