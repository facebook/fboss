// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <gtest/gtest.h>

#include "fboss/platform/bsp_tests/cpp/BspTest.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

BspTestEnvironment* BspTest::env_ = nullptr;

void BspTest::recordExpectedError(
    const std::string& testName,
    const std::string& deviceName,
    const std::string& reason) {
  env_->recordExpectedError(testName, deviceName, reason);
}

TEST_F(BspTest, UsingFixture) {
  const auto& config = GetPlatformManagerConfig();
  EXPECT_FALSE(config.platformName()->empty())
      << "Platform name should not be empty";
}

} // namespace facebook::fboss::platform::bsp_tests::cpp
