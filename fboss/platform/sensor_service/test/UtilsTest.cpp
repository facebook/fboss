// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/sensor_service/Utils.h"

namespace facebook::fboss::platform::sensor_service {
TEST(computeExpressionTests, Equal) {
  EXPECT_FLOAT_EQ(Utils::computeExpression("x * 0.1", 10.0), 1.0);
  EXPECT_FLOAT_EQ(Utils::computeExpression("x * 0.1/100", 10.0), 0.01);
  EXPECT_FLOAT_EQ(
      Utils::computeExpression("(x * 0.1+5)/1000", 100.0, "x"), 0.015);
  EXPECT_FLOAT_EQ(
      Utils::computeExpression("(y / 0.1+300)/ (1000*10 + 5)", 30.0, "y"),
      0.05997);
  EXPECT_FLOAT_EQ(
      Utils::computeExpression("(@ / 0.1+300)/ (1000*10 + 5)", 30.0, "x"),
      0.05997);
  EXPECT_FLOAT_EQ(
      Utils::computeExpression(
          "(@ / 0.1+300)/ (1000*10 + @ * 10000)", 30.0, "x"),
      0.0019354839);
}
} // namespace facebook::fboss::platform::sensor_service
