// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <gtest/gtest.h>
#include <optional>

#include "fboss/platform/bsp_tests/cpp/BspTestEnvironment.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

// Base test fixture that provides access to the environment
class BspTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    env_ = BspTestEnvironment::GetInstance();
    ASSERT_NE(env_, nullptr) << "BspTestEnvironment instance is null";
  }

  const RuntimeConfig& GetRuntimeConfig() const {
    return env_->getRuntimeConfig();
  }

  const BspTestsConfig& GetTestConfig() const {
    return env_->getTestConfig();
  }

  const platform_manager::PlatformConfig& GetPlatformManagerConfig() const {
    return env_->getPlatformManagerConfig();
  }

  const std::optional<DeviceTestData> getDeviceTestData(
      const I2CDevice& device) const {
    RuntimeConfig conf = GetRuntimeConfig();
    if (conf.testData()->contains(*device.pmName())) {
      return conf.testData()->at(*device.pmName());
    }
    return std::nullopt;
  }

  static BspTestEnvironment* env_;
};

} // namespace facebook::fboss::platform::bsp_tests::cpp
