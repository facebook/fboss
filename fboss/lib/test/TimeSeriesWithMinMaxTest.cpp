// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/lib/TimeSeriesWithMinMax.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace facebook::fboss;

using namespace std::chrono;
using namespace std::this_thread;

TEST(TimeSeriesWithMinMax, BasicTest) {
  TimeSeriesWithMinMax<int> buffer(seconds(3), seconds(1));

  buffer.addValue(5);
  buffer.addValue(7);
  buffer.addValue(3);
  EXPECT_EQ(buffer.getMax(), 7);
  EXPECT_EQ(buffer.getMin(), 3);

  /* sleep override */
  sleep_for(seconds(3));
  buffer.addValue(2);
  buffer.addValue(5, std::chrono::system_clock::now() - seconds(5));
  EXPECT_EQ(buffer.getMax(), 2);
  EXPECT_EQ(buffer.getMin(), 2);
  buffer.addValue(3, std::chrono::system_clock::now() - seconds(2));
  EXPECT_EQ(buffer.getMax(), 3);
  EXPECT_EQ(buffer.getMin(), 2);
  buffer.addValue(4, std::chrono::system_clock::now());
  EXPECT_EQ(buffer.getMax(), 4);
  EXPECT_EQ(buffer.getMin(), 2);

  EXPECT_EQ(
      buffer.getMax(
          std::chrono::system_clock::now() - seconds(3),
          std::chrono::system_clock::now() - seconds(2)),
      3);
  EXPECT_EQ(
      buffer.getMin(
          std::chrono::system_clock::now() - seconds(3),
          std::chrono::system_clock::now() - seconds(2)),
      3);

  /* sleep override */
  sleep_for(seconds(3));
  try {
    buffer.getMax();
    assert(false);
  } catch (std::runtime_error& e) {
  }

  buffer.addValue(1);
  buffer.addValue(2);
  buffer.addValue(3);
  EXPECT_EQ(buffer.getMax(), 3);
  /* sleep override */
  sleep_for(seconds(1));

  buffer.addValue(4);
  buffer.addValue(5);
  EXPECT_EQ(buffer.getMax(), 5);
  /* sleep override */
  sleep_for(seconds(3));
  try {
    buffer.getMax();
    assert(false);
  } catch (std::runtime_error& e) {
  }
}
