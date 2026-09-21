/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/MapDelta.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class MapDeltaTest : public ::testing::Test {
 public:
  void SetUp() override {
    added.clear();
    removed.clear();
    changed.clear();
  }
  void computeDelta(
      const std::map<int, std::string>& oldMap,
      const std::map<int, std::string>& newMap) {
    DeltaFunctions::forEachChanged(
        MapDelta(&oldMap, &newMap),
        [&](auto, auto newStr) { changed.push_back(*newStr); },
        [&](auto addStr) { added.push_back(*addStr); },
        [&](auto rmStr) { removed.push_back(*rmStr); });
  }
  void computeDelta(
      const std::map<int, std::shared_ptr<std::string>>& oldMap,
      const std::map<int, std::shared_ptr<std::string>>& newMap) {
    DeltaFunctions::forEachChanged(
        MapDelta(&oldMap, &newMap),
        [&](auto, auto newStr) { changed.push_back(*newStr); },
        [&](auto addStr) { added.push_back(*addStr); },
        [&](auto rmStr) { removed.push_back(*rmStr); });
  }
  std::vector<std::string> added, removed, changed;
};

TEST_F(MapDeltaTest, addRemoveChange) {
  std::map<int, std::string> oldMap{{1, "one"}, {2, "too"}};
  std::map<int, std::string> newMap{{2, "two"}, {3, "three"}};
  computeDelta(oldMap, newMap);
  EXPECT_EQ(added, std::vector<std::string>({newMap[3]}));
  EXPECT_EQ(removed, std::vector<std::string>({oldMap[1]}));
  EXPECT_EQ(changed, std::vector<std::string>({newMap[2]}));
}

TEST_F(MapDeltaTest, addRemove) {
  std::map<int, std::string> oldMap{{1, "one"}};
  std::map<int, std::string> newMap{{2, "two"}};
  computeDelta(oldMap, newMap);
  EXPECT_EQ(added, std::vector<std::string>({newMap[2]}));
  EXPECT_EQ(removed, std::vector<std::string>({oldMap[1]}));
  EXPECT_EQ(changed.size(), 0);
}

TEST_F(MapDeltaTest, addOnly) {
  std::map<int, std::string> oldMap{{1, "one"}};
  std::map<int, std::string> newMap{{1, "one"}, {2, "two"}};
  computeDelta(oldMap, newMap);
  EXPECT_EQ(added, std::vector<std::string>({newMap[2]}));
  EXPECT_EQ(removed.size(), 0);
  EXPECT_EQ(changed.size(), 0);
}

TEST_F(MapDeltaTest, removeOnly) {
  std::map<int, std::string> oldMap{{1, "one"}};
  std::map<int, std::string> newMap;
  computeDelta(oldMap, newMap);
  EXPECT_EQ(added.size(), 0);
  EXPECT_EQ(removed, std::vector<std::string>({oldMap[1]}));
  EXPECT_EQ(changed.size(), 0);
}

TEST_F(MapDeltaTest, changeOnly) {
  std::map<int, std::string> oldMap{{1, "1"}};
  std::map<int, std::string> newMap{{1, "one"}};
  computeDelta(oldMap, newMap);
  EXPECT_EQ(added.size(), 0);
  EXPECT_EQ(removed.size(), 0);
  EXPECT_EQ(changed, std::vector<std::string>({newMap[1]}));
}

TEST_F(MapDeltaTest, noChange) {
  std::map<int, std::string> oldMap{{1, "one"}};
  std::map<int, std::string> newMap(oldMap);
  computeDelta(oldMap, newMap);
  EXPECT_EQ(added.size(), 0);
  EXPECT_EQ(removed.size(), 0);
  EXPECT_EQ(changed.size(), 0);
}

TEST_F(MapDeltaTest, comapreDeep) {
  std::map<int, std::shared_ptr<std::string>> oldMap{
      {1, std::make_shared<std::string>("one")}};
  std::map<int, std::shared_ptr<std::string>> newMap{
      {1, std::make_shared<std::string>("one")}};
  EXPECT_EQ(DeltaComparison::policy(), DeltaComparison::Policy::SHALLOW);
  {
    DeltaComparison::PolicyRAII policyGuard{DeltaComparison::Policy::DEEP};
    computeDelta(oldMap, newMap);
    EXPECT_EQ(added.size(), 0);
    EXPECT_EQ(removed.size(), 0);
    EXPECT_EQ(changed.size(), 0);
  }
  EXPECT_EQ(DeltaComparison::policy(), DeltaComparison::Policy::SHALLOW);
}

TEST_F(MapDeltaTest, comapreShallow) {
  std::string oldStr{"one"};
  std::string newStr{"one"};
  std::map<int, std::shared_ptr<std::string>> oldMap{
      {1, std::make_shared<std::string>("one")}};
  std::map<int, std::shared_ptr<std::string>> newMap{
      {1, std::make_shared<std::string>("one")}};
  computeDelta(oldMap, newMap);
  EXPECT_EQ(DeltaComparison::policy(), DeltaComparison::Policy::SHALLOW);
  EXPECT_EQ(added.size(), 0);
  EXPECT_EQ(removed.size(), 0);
  EXPECT_EQ(changed.size(), 1);
}
