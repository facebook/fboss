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

  std::tuple<I2CAdapter, I2CDevice> getI2cAdapterAndDevice(std::string pmName);

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
    cleanupDevices();
  }

  // Helper to register a device for cleanup
  void registerDeviceForCleanup(
      const PciDeviceInfo& pciDevice,
      const fbiob::AuxData& auxDevice,
      const int id) {
    devicesToCleanup_.push_back({pciDevice, auxDevice, id});
  }

  void registerAdapterForCleanup(const I2CAdapter& adapter, int id) {
    if (adapter.pciAdapterInfo().has_value()) {
      registerDeviceForCleanup(
          *adapter.pciAdapterInfo()->pciInfo(),
          *adapter.pciAdapterInfo()->auxData(),
          id);
    }
  }

  void cleanupDevices() {
    for (const auto& device : devicesToCleanup_) {
      try {
        CdevUtils::deleteDevice(device.pciDevice, device.auxDevice, device.id);
      } catch (const std::exception& e) {
        XLOG(ERR) << "Failed to delete device during cleanup: " << e.what();
      }
    }
    devicesToCleanup_.clear();
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

  const std::optional<std::string> getExpectedErrorReason(
      const std::string& pmName,
      ExpectedErrorType type) const {
    RuntimeConfig conf = GetRuntimeConfig();
    if (conf.expectedErrors()->contains(pmName)) {
      const auto& errors = conf.expectedErrors()->at(pmName);
      if (errors.contains(type)) {
        return errors.at(type);
      }
    }
    return std::nullopt;
  }

  std::string getCurrentTestName() const {
    const auto* test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    return test_info ? test_info->name() : "UnknownTest";
  }

  // Record an error that occurred during testing
  void recordExpectedError(
      const std::string& testName,
      const std::string& deviceName,
      const std::string& reason);

  // Overloaded recordError that automatically gets test name
  void recordExpectedError(
      const std::string& deviceName,
      const std::string& reason) {
    recordExpectedError(getCurrentTestName(), deviceName, reason);
  }

  static BspTestEnvironment* env_;
};

} // namespace facebook::fboss::platform::bsp_tests::cpp
