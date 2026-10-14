// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/OverrideSetIdManager.h"

namespace facebook::fboss {

namespace {
NextHopIDSet idSet(std::initializer_list<int64_t> ids) {
  NextHopIDSet out;
  for (auto id : ids) {
    out.insert(NextHopID(id));
  }
  return out;
}
} // namespace

class OverrideSetIdManagerTest : public ::testing::Test {
 protected:
  OverrideSetIdManager mgr_;
};

TEST_F(OverrideSetIdManagerTest, overrideRangeFollowsRibRange) {
  EXPECT_FALSE(OverrideSetIdManager::isOverrideSetID(NextHopSetID(1LL << 62)));
  EXPECT_FALSE(
      OverrideSetIdManager::isOverrideSetID(
          NextHopSetID((1LL << 62) + (1LL << 60) - 1)));
  EXPECT_TRUE(
      OverrideSetIdManager::isOverrideSetID(
          NextHopSetID((1LL << 62) + (1LL << 60))));
  EXPECT_TRUE(
      OverrideSetIdManager::isOverrideSetID(
          NextHopSetID((1LL << 62) + (1LL << 61) - 1)));
  EXPECT_FALSE(
      OverrideSetIdManager::isOverrideSetID(
          NextHopSetID((1LL << 62) + (1LL << 61))));
}

TEST_F(OverrideSetIdManagerTest, getOrAllocatePopulatesIdToMembers) {
  const auto start = nextHopIdTypeStart(NextHopIdType::OVERRIDE_SET);

  EXPECT_THROW(mgr_.getOrAllocate(NextHopIDSet{}), FbossError);
  EXPECT_TRUE(mgr_.getIdToMembers().empty());

  auto a = mgr_.getOrAllocate(idSet({1, 9}))->second;
  EXPECT_TRUE(OverrideSetIdManager::isOverrideSetID(a));
  EXPECT_EQ(static_cast<int64_t>(a), start);
  EXPECT_EQ(mgr_.getOrAllocate(idSet({9, 1}))->second, a);

  auto b = mgr_.getOrAllocate(idSet({1, 3}))->second;
  auto c = mgr_.getOrAllocate(idSet({42}))->second;
  EXPECT_EQ(static_cast<int64_t>(b), start + 1);
  EXPECT_EQ(static_cast<int64_t>(c), start + 2);

  EXPECT_EQ(mgr_.getIdToMembers().size(), 3);
  EXPECT_EQ(mgr_.getMembersToId().size(), 3);
  EXPECT_EQ(mgr_.getIdToMembers().at(a), idSet({1, 9}));
  EXPECT_EQ(mgr_.getIdToMembers().at(b), idSet({1, 3}));
  EXPECT_EQ(mgr_.getIdToMembers().at(c), idSet({42}));
}

TEST_F(OverrideSetIdManagerTest, deallocOverrideSetId) {
  auto a = mgr_.getOrAllocate(idSet({1, 2}))->second;
  auto b = mgr_.getOrAllocate(idSet({3, 4}))->second;

  EXPECT_FALSE(mgr_.deallocOverrideSetId(
      NextHopSetID(nextHopIdTypeStart(NextHopIdType::OVERRIDE_SET) + 100)));
  EXPECT_EQ(mgr_.getIdToMembers().size(), 2);

  EXPECT_TRUE(mgr_.deallocOverrideSetId(a));
  EXPECT_FALSE(mgr_.deallocOverrideSetId(a));
  EXPECT_EQ(mgr_.getIdToMembers().size(), 1);
  EXPECT_EQ(mgr_.getMembersToId().size(), 1);
  EXPECT_FALSE(mgr_.getIdToMembers().contains(a));
  EXPECT_EQ(mgr_.getIdToMembers().at(b), idSet({3, 4}));

  auto aAgain = mgr_.getOrAllocate(idSet({1, 2}))->second;
  EXPECT_GT(static_cast<int64_t>(aAgain), static_cast<int64_t>(b));

  EXPECT_TRUE(mgr_.deallocOverrideSetId(b));
  EXPECT_TRUE(mgr_.deallocOverrideSetId(aAgain));
  EXPECT_TRUE(mgr_.getIdToMembers().empty());
  EXPECT_TRUE(mgr_.getMembersToId().empty());
}

} // namespace facebook::fboss
