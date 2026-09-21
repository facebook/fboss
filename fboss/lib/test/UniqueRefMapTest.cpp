// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/UniqueRefMap.h"

#include <gtest/gtest.h>
#include <string>

using namespace facebook::fboss;

TEST(UniqueRefMap, RefNewEntry) {
  UniqueRefMap<int, std::string> map;
  EXPECT_TRUE(map.ref(1, "a"));
  EXPECT_EQ(map.size(), 1);
}

TEST(UniqueRefMap, RefSameValue) {
  UniqueRefMap<int, std::string> map;
  EXPECT_TRUE(map.ref(1, "a"));
  EXPECT_TRUE(map.ref(1, "a"));
  EXPECT_EQ(map.size(), 1);
}

TEST(UniqueRefMap, RefDifferentValue) {
  UniqueRefMap<int, std::string> map;
  EXPECT_TRUE(map.ref(1, "a"));
  EXPECT_FALSE(map.ref(1, "b"));
  EXPECT_EQ(map.size(), 1);
  // Original value unchanged
  EXPECT_EQ(*map.get(1), "a");
}

TEST(UniqueRefMap, RefDifferentKeys) {
  UniqueRefMap<int, std::string> map;
  EXPECT_TRUE(map.ref(1, "a"));
  EXPECT_TRUE(map.ref(2, "b"));
  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(*map.get(1), "a");
  EXPECT_EQ(*map.get(2), "b");
}

TEST(UniqueRefMap, UnrefDecrement) {
  UniqueRefMap<int, std::string> map;
  map.ref(1, "a");
  map.ref(1, "a");
  EXPECT_FALSE(map.unref(1));
  EXPECT_EQ(map.size(), 1);
}

TEST(UniqueRefMap, UnrefRemove) {
  UniqueRefMap<int, std::string> map;
  map.ref(1, "a");
  EXPECT_TRUE(map.unref(1));
  EXPECT_EQ(map.size(), 0);
  EXPECT_TRUE(map.empty());
}

TEST(UniqueRefMap, UnrefNonExistent) {
  UniqueRefMap<int, std::string> map;
  EXPECT_THROW(map.unref(1), std::out_of_range);
}

TEST(UniqueRefMap, GetExisting) {
  UniqueRefMap<int, std::string> map;
  map.ref(1, "a");
  const auto* val = map.get(1);
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(*val, "a");
}

TEST(UniqueRefMap, GetNonExistent) {
  UniqueRefMap<int, std::string> map;
  EXPECT_EQ(map.get(1), nullptr);
}

TEST(UniqueRefMap, Empty) {
  UniqueRefMap<int, std::string> map;
  EXPECT_TRUE(map.empty());
  map.ref(1, "a");
  EXPECT_FALSE(map.empty());
}

TEST(UniqueRefMap, Clear) {
  UniqueRefMap<int, std::string> map;
  map.ref(1, "a");
  map.ref(2, "b");
  EXPECT_EQ(map.size(), 2);
  map.clear();
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
}

TEST(UniqueRefMap, ReinsertAfterFullUnref) {
  UniqueRefMap<int, std::string> map;
  map.ref(1, "a");
  map.unref(1);
  // Key removed, can now insert with different value
  EXPECT_TRUE(map.ref(1, "b"));
  EXPECT_EQ(*map.get(1), "b");
}

TEST(UniqueRefMap, EraseExisting) {
  UniqueRefMap<int, std::string> map;
  map.ref(1, "a");
  map.ref(1, "a");
  map.erase(1);
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.get(1), nullptr);
}

TEST(UniqueRefMap, EraseNonExistent) {
  UniqueRefMap<int, std::string> map;
  // Should not throw
  map.erase(42);
  EXPECT_TRUE(map.empty());
}

TEST(UniqueRefMap, SetNewEntry) {
  UniqueRefMap<int, std::string> map;
  map.set(1, "a", 3);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(*map.get(1), "a");
  // refcount is 3, so 2 unrefs should not remove
  EXPECT_FALSE(map.unref(1));
  EXPECT_FALSE(map.unref(1));
  EXPECT_TRUE(map.unref(1));
  EXPECT_TRUE(map.empty());
}

TEST(UniqueRefMap, SetOverwriteExisting) {
  UniqueRefMap<int, std::string> map;
  map.ref(1, "a");
  map.set(1, "b", 2);
  EXPECT_EQ(*map.get(1), "b");
  EXPECT_FALSE(map.unref(1));
  EXPECT_TRUE(map.unref(1));
  EXPECT_TRUE(map.empty());
}

TEST(UniqueRefMap, EraseAndReinsert) {
  UniqueRefMap<int, std::string> map;
  map.ref(1, "a");
  map.erase(1);
  EXPECT_TRUE(map.ref(1, "b"));
  EXPECT_EQ(*map.get(1), "b");
}

TEST(UniqueRefMap, MultipleRefsFullUnref) {
  UniqueRefMap<int, std::string> map;
  map.ref(1, "a");
  map.ref(1, "a");
  map.ref(1, "a");
  EXPECT_FALSE(map.unref(1)); // 3 -> 2
  EXPECT_FALSE(map.unref(1)); // 2 -> 1
  EXPECT_TRUE(map.unref(1)); // 1 -> 0, removed
  EXPECT_TRUE(map.empty());
}
