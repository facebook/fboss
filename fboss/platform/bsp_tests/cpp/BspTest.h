// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>

#include "fboss/platform/bsp_tests/cpp/BspTestEnvironment.h"
#include "fboss/platform/bsp_tests/cpp/utils/CdevUtils.h"
#include "fboss/platform/bsp_tests/cpp/utils/KmodUtils.h"
#include "fboss/platform/bsp_tests/gen-cpp2/bsp_tests_config_types.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

// Base test fixture that provides access to the environment
class BspTest : public ::testing::Test {
 public:
  // Static methods for loading and unloading kmods that can be used in
  // SetUpTestSuite
  static void loadKmods() {
    env_ = BspTestEnvironment::GetInstance();
    ASSERT_NE(env_, nullptr) << "BspTestEnvironment instance is null";
    KmodUtils::loadKmods(*env_->getRuntimeConfig().kmods());
  }

  static void unloadKmods() {
    env_ = BspTestEnvironment::GetInstance();
    ASSERT_NE(env_, nullptr) << "BspTestEnvironment instance is null";
    KmodUtils::unloadKmods(*env_->getRuntimeConfig().kmods());
  }

 protected:
  static void SetUpTestSuite() {
    env_ = BspTestEnvironment::GetInstance();
    ASSERT_NE(env_, nullptr) << "BspTestEnvironment instance is null";
    BspTest::unloadKmods();
    BspTest::loadKmods();
  }

  void SetUp() override {
    BspTest::loadKmods();
  }

  // Track devices created during tests for automatic cleanup
  struct DeviceToCleanup {
    PciDeviceInfo pciDevice;
    fbiob::AuxData auxDevice;
    int id;
  };
  std::vector<DeviceToCleanup> devicesToCleanup_;

  void TearDown() override {
    // Clean up all created devices
    for (const auto& device : devicesToCleanup_) {
      try {
        CdevUtils::deleteDevice(device.pciDevice, device.auxDevice, device.id);
      } catch (const std::exception& e) {
        XLOG(ERR) << "Failed to delete device during cleanup: " << e.what();
      }
    }
    devicesToCleanup_.clear();
  }

  // Helper to register a device for cleanup
  void registerDeviceForCleanup(
      const PciDeviceInfo& pciDevice,
      const fbiob::AuxData& auxDevice,
      const int id) {
    devicesToCleanup_.push_back({pciDevice, auxDevice, id});
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
