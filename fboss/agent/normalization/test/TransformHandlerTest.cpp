/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/normalization/TransformHandler.h"

using namespace ::testing;

namespace facebook::fboss::normalization {

class TransformHandlerTest : public ::testing::Test {
 public:
  TransformHandlerTest() {}

  TransformHandler handler;
};

TEST_F(TransformHandlerTest, rate) {
  EXPECT_FALSE(
      handler.rate("eth0", "rate_property_a", StatTimestamp(1000), 500, 5)
          .has_value());
  EXPECT_FALSE(
      handler.rate("eth0", "rate_property_b", StatTimestamp(1000), 600, 5)
          .has_value());
  EXPECT_FALSE(
      handler.rate("eth1", "rate_property_a", StatTimestamp(1000), 700, 5)
          .has_value());

  EXPECT_DOUBLE_EQ(
      *handler.rate("eth0", "rate_property_a", StatTimestamp(1010), 600, 5),
      10);
  EXPECT_DOUBLE_EQ(
      *handler.rate("eth0", "rate_property_b", StatTimestamp(1010), 700, 5),
      10);
  EXPECT_DOUBLE_EQ(
      *handler.rate("eth1", "rate_property_a", StatTimestamp(1010), 800, 5),
      10);

  EXPECT_DOUBLE_EQ(
      *handler.rate("eth0", "rate_property_a", StatTimestamp(1020), 750, 5),
      15);
  EXPECT_DOUBLE_EQ(
      *handler.rate("eth0", "rate_property_b", StatTimestamp(1020), 850, 5),
      15);
  EXPECT_DOUBLE_EQ(
      *handler.rate("eth1", "rate_property_a", StatTimestamp(1020), 950, 5),
      15);
}

TEST_F(TransformHandlerTest, bps) {
  EXPECT_FALSE(
      handler.bps("eth0", "bps_property_a", StatTimestamp(1000), 500, 5)
          .has_value());
  EXPECT_FALSE(
      handler.bps("eth0", "bps_property_b", StatTimestamp(1000), 600, 5)
          .has_value());
  EXPECT_FALSE(
      handler.bps("eth1", "bps_property_a", StatTimestamp(1000), 700, 5)
          .has_value());

  EXPECT_DOUBLE_EQ(
      *handler.bps("eth0", "bps_property_a", StatTimestamp(1010), 600, 5), 80);
  EXPECT_DOUBLE_EQ(
      *handler.bps("eth0", "bps_property_b", StatTimestamp(1010), 700, 5), 80);
  EXPECT_DOUBLE_EQ(
      *handler.bps("eth1", "bps_property_a", StatTimestamp(1010), 800, 5), 80);

  EXPECT_DOUBLE_EQ(
      *handler.bps("eth0", "bps_property_a", StatTimestamp(1020), 750, 5), 120);
  EXPECT_DOUBLE_EQ(
      *handler.bps("eth0", "bps_property_b", StatTimestamp(1020), 850, 5), 120);
  EXPECT_DOUBLE_EQ(
      *handler.bps("eth1", "bps_property_a", StatTimestamp(1020), 950, 5), 120);
}

} // namespace facebook::fboss::normalization
