// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <string>

#include <folly/json/json.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/bsp_tests/gen-cpp2/bsp_tests_config_types.h"
#include "fboss/platform/platform_manager/PkgManager.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

/**
 * Global test environment for BSP tests.
 * This class is responsible for loading and parsing configuration files,
 * and providing access to the runtime configuration.
 */
class BspTestEnvironment : public ::testing::Environment {
 public:
  virtual ~BspTestEnvironment() override;

  BspTestEnvironment(const BspTestEnvironment&) = delete;
  BspTestEnvironment(BspTestEnvironment&&) = delete;
  BspTestEnvironment& operator=(const BspTestEnvironment&) = delete;
  BspTestEnvironment& operator=(BspTestEnvironment&&) = delete;

  void SetUp() override;
  void TearDown() override;

  const platform_manager::PlatformConfig& getPlatformManagerConfig() const;
  const BspTestsConfig& getTestConfig() const;
  const RuntimeConfig& getRuntimeConfig() const;

  // Static accessor method for the singleton instance
  static BspTestEnvironment* GetInstance(const std::string& platform = "") {
    if (instance_ == nullptr && !platform.empty()) {
      instance_ = new BspTestEnvironment(platform);
    }
    return instance_;
  }

 private:
  // Private constructor to enforce singleton pattern
  explicit BspTestEnvironment(const std::string& platform);

  platform_manager::PlatformConfig loadPlatformManagerConfig();

  BspTestsConfig loadTestConfig(const std::string& platform);

  // Static instance pointer for singleton pattern
  static BspTestEnvironment* instance_;

  std::string platform_;
  BspTestsConfig testConfig_;
  platform_manager::PlatformConfig platformManagerConfig_;
  RuntimeConfig runtimeConfig_;
  platform_manager::BspKmodsFile kmods_;
  std::unique_ptr<platform_manager::PkgManager> pkgManager_;
};

} // namespace facebook::fboss::platform::bsp_tests::cpp
