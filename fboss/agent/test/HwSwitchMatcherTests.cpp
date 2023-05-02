// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/HwSwitchMatcher.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

TEST(NpuMatcherTest, NpuIds) {
  HwSwitchMatcher matcher("id=2013,10286,40176");

  EXPECT_TRUE(matcher.has(SwitchID(10286)));
  EXPECT_TRUE(matcher.has(SwitchID(2013)));
  EXPECT_TRUE(matcher.has(SwitchID(40176)));

  EXPECT_FALSE(matcher.has(SwitchID(1)));
  EXPECT_FALSE(matcher.has(SwitchID(2)));
  EXPECT_FALSE(matcher.has(SwitchID(6)));
}

TEST(NpuMatcherTest, MatcherString) {
  HwSwitchMatcher matcher0("id=2013,10286,40176");
  HwSwitchMatcher matcher1(matcher0.switchIds());
  HwSwitchMatcher matcher2(matcher1.matcherString());

  EXPECT_EQ(matcher0.switchIds(), matcher1.switchIds());
  EXPECT_EQ(matcher1.switchIds(), matcher2.switchIds());

  // will match if npu ids are listed in ascending order only
  EXPECT_EQ(matcher1.matcherString(), matcher2.matcherString());
}

TEST(NpuMatcherTest, DefaultMatcher) {
  HwSwitchMatcher matcher;
  EXPECT_TRUE(matcher.has(SwitchID(0)));
  EXPECT_EQ(matcher.matcherString(), "id=0");
  EXPECT_EQ(HwSwitchMatcher::defaultHwSwitchMatcher().matcherString(), "id=0");
}

TEST(NpuMatcherTest, Compare) {
  HwSwitchMatcher matcher1(std::unordered_set<SwitchID>{SwitchID(1)});
  HwSwitchMatcher matcher2(std::unordered_set<SwitchID>{SwitchID(2)});
  EXPECT_NE(matcher1, matcher2);
  EXPECT_EQ(matcher1, matcher1);
  EXPECT_EQ(matcher2, matcher2);
}
