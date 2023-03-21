// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/NpuMatcher.h"

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
  HwSwitchMatcher matcher1(matcher0.npus());
  HwSwitchMatcher matcher2(matcher1.matcherString());

  EXPECT_EQ(matcher0.npus(), matcher1.npus());
  EXPECT_EQ(matcher1.npus(), matcher2.npus());

  // will match if npu ids are listed in ascending order only
  EXPECT_EQ(matcher1.matcherString(), matcher2.matcherString());
}

TEST(NpuMatcherTest, DefaultMatcher) {
  HwSwitchMatcher matcher;
  EXPECT_TRUE(matcher.has(SwitchID(0)));
  EXPECT_EQ(matcher.matcherString(), "id=0");
  EXPECT_EQ(HwSwitchMatcher::defaultHwSwitchMatcher().matcherString(), "id=0");
}
