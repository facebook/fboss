// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

#include "fboss/platform/bsp_tests/cpp/BspTest.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

BspTestEnvironment* BspTest::env_ = nullptr;

TEST_F(BspTest, UsingFixture) {
  const auto& config = GetPlatformManagerConfig();
  XLOG(INFO) << "Platform from fixture: " << *config.platformName();

  // Perform your test assertions
  EXPECT_FALSE(config.platformName()->empty())
      << "Platform name should not be empty";
}

} // namespace facebook::fboss::platform::bsp_tests::cpp
