// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <gtest/gtest.h>

#include "fboss/platform/bsp_tests/BspTest.h"

namespace facebook::fboss::platform::bsp_tests {

BspTestEnvironment* BspTest::env_ = nullptr;

void BspTest::recordExpectedError(
    const std::string& testName,
    const std::string& deviceName,
    const std::string& reason) {
  env_->recordExpectedError(testName, deviceName, reason);
}

std::tuple<I2CAdapter, I2CDevice> BspTest::getI2cAdapterAndDevice(
    std::string pmName) {
  for (const auto& [_, adapter] : *GetRuntimeConfig().i2cAdapters()) {
    for (const auto& i2cDevice : *adapter.i2cDevices()) {
      if (*i2cDevice.pmName() == pmName) {
        return std::make_tuple(adapter, i2cDevice);
      }
    }
  }
  throw std::runtime_error(
      fmt::format("No I2C device found with pmName {}", pmName));
}

TEST_F(BspTest, UsingFixture) {
  const auto& config = GetPlatformManagerConfig();
  EXPECT_FALSE(config.platformName()->empty())
      << "Platform name should not be empty";
}

} // namespace facebook::fboss::platform::bsp_tests
