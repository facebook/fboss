/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/CounterUtils.h"
#include "fboss/agent/hw/bcm/gen-cpp2/hardware_stats_constants.h"

#include <gtest/gtest.h>
using namespace facebook::fboss;
using namespace facebook::fboss::utility;
namespace {
auto constexpr kUninit = hardware_stats_constants::STAT_UNINITIALIZED();
}
TEST(CounterUtilsTests, PrevUninit) {
  EXPECT_EQ(0, getDerivedCounterIncrement(kUninit, 0, 0, 0));
  EXPECT_EQ(0, getDerivedCounterIncrement(0, 0, kUninit, 0));
}

TEST(CounterUtilsTests, CurUninit) {
  ASSERT_DEATH(getDerivedCounterIncrement(0, kUninit, 0, 0), ".*uninitialized");
  ASSERT_DEATH(getDerivedCounterIncrement(0, 0, 0, kUninit), ".*uninitialized");
}

TEST(CounterUtilsTests, Rollover) {
  EXPECT_EQ(0, getDerivedCounterIncrement(100, 1, 10, 15));
  EXPECT_EQ(0, getDerivedCounterIncrement(0, 0, 100, 1));
}

TEST(CounterUtilsTests, ComputeIncrement) {
  // 100% increment came from to be subtracted counter
  EXPECT_EQ(0, getDerivedCounterIncrement(10, 15, 10, 15));
  // Greater than 100% increment came from to be subtracted counter
  EXPECT_EQ(0, getDerivedCounterIncrement(10, 15, 10, 20));
  // Less than 100% increment came from to be subtracted counter
  EXPECT_EQ(1, getDerivedCounterIncrement(10, 15, 10, 14));
}

TEST(CounterUtilsTests, PrevUninitMultipleToSub) {
  EXPECT_EQ(
      0,
      getDerivedCounterIncrement(
          CounterPrevAndCur{kUninit, 0},
          {CounterPrevAndCur{0, 0}, CounterPrevAndCur{0, 0}}));
  EXPECT_EQ(
      0,
      getDerivedCounterIncrement(
          CounterPrevAndCur{0, 0},
          {CounterPrevAndCur{0, 0}, CounterPrevAndCur{kUninit, 0}}));
}
// Multiple counters
TEST(CounterUtilsTests, CurUninitMultipleToSub) {
  ASSERT_DEATH(
      getDerivedCounterIncrement(
          CounterPrevAndCur{0, kUninit},
          {CounterPrevAndCur{0, 0}, CounterPrevAndCur{0, 0}}),
      ".*uninitialized");
  ASSERT_DEATH(
      getDerivedCounterIncrement(
          CounterPrevAndCur{0, 0}, {{0, 0}, {0, kUninit}}),
      ".*uninitialized");
}

TEST(CounterUtilsTests, RolloverMultipleToSub) {
  EXPECT_EQ(
      0,
      getDerivedCounterIncrement(
          CounterPrevAndCur{100, 1}, {{0, 0}, {10, 15}}));

  EXPECT_EQ(
      0,
      getDerivedCounterIncrement(CounterPrevAndCur{0, 0}, {{0, 0}, {100, 1}}));
}

TEST(CounterUtilsTests, ComputeIncrementMultipleToSub) {
  // 100% increment came from to be subtracted counter
  EXPECT_EQ(
      0,
      getDerivedCounterIncrement(
          CounterPrevAndCur{20, 30}, {{10, 15}, {10, 15}}));

  // Greater than 100% increment came from to be subtracted counter
  EXPECT_EQ(
      0,
      getDerivedCounterIncrement(
          CounterPrevAndCur{20, 30}, {{10, 16}, {10, 15}}));
  // Less than 100% increment came from to be subtracted counter
  EXPECT_EQ(
      2,
      getDerivedCounterIncrement(
          CounterPrevAndCur{20, 30}, {{10, 14}, {10, 14}}));
  EXPECT_EQ(
      1,
      getDerivedCounterIncrement(
          CounterPrevAndCur{20, 30}, {{10, 10}, {10, 19}}));
  EXPECT_EQ(
      1,
      getDerivedCounterIncrement(
          CounterPrevAndCur{20, 30}, {{10, 19}, {10, 10}}));
}
