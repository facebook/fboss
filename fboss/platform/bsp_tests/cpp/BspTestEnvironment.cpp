// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/platform/bsp_tests/cpp/BspTestEnvironment.h"

#include <folly/logging/xlog.h>

#include "fboss/platform/bsp_tests/cpp/RuntimeConfigBuilder.h"
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/platform_manager/ConfigUtils.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

// Initialize the static instance pointer
BspTestEnvironment* BspTestEnvironment::instance_ = nullptr;

BspTestEnvironment::~BspTestEnvironment() {}

void BspTestEnvironment::SetUp() {}

void BspTestEnvironment::TearDown() {}

BspTestEnvironment::BspTestEnvironment(const std::string& platform) {
  platform_ = platform;

  // Load bsp test config
  testConfig_ = loadTestConfig(platform_);

  // Load platform manager configuration
  platformManagerConfig_ = loadPlatformManagerConfig();

  pkgManager_ =
      std::make_unique<platform_manager::PkgManager>(platformManagerConfig_);
  kmods_ = pkgManager_->readKmodsFile();

  // Build runtime configuration using RuntimeConfigBuilder
  RuntimeConfigBuilder configBuilder;
  runtimeConfig_ = configBuilder.buildRuntimeConfig(
      testConfig_, platformManagerConfig_, kmods_, platform_);
}

const platform_manager::PlatformConfig&
BspTestEnvironment::getPlatformManagerConfig() const {
  return platformManagerConfig_;
}

const BspTestsConfig& BspTestEnvironment::getTestConfig() const {
  return testConfig_;
}

const RuntimeConfig& BspTestEnvironment::getRuntimeConfig() const {
  return runtimeConfig_;
}

platform_manager::PlatformConfig
BspTestEnvironment::loadPlatformManagerConfig() {
  return platform::platform_manager::ConfigUtils().getConfig();
}

BspTestsConfig BspTestEnvironment::loadTestConfig(const std::string& platform) {
  std::string configJson = ConfigLib().getBspTestConfig(platform);
  BspTestsConfig config;
  try {
    apache::thrift::SimpleJSONSerializer::deserialize<BspTestsConfig>(
        configJson, config);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to deserialize test config: " << e.what();
    throw;
  }
  XLOG(DBG2) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      config);
  return config;
}

void BspTestEnvironment::recordExpectedError(
    const std::string& testName,
    const std::string& deviceName,
    const std::string& reason) {
  RecordedError error;
  error.testName = testName;
  error.deviceName = deviceName;
  error.reason = reason;

  recordedErrors_.push_back(error);
}

void BspTestEnvironment::printAllRecordedErrors() const {
  if (recordedErrors_.empty()) {
    return;
  }

  XLOG(INFO) << "=== EXPECTED ERRORS SUMMARY ===";
  XLOG(INFO) << "Total errors recorded: " << recordedErrors_.size();

  for (size_t i = 0; i < recordedErrors_.size(); ++i) {
    const auto& error = recordedErrors_[i];
    XLOG(INFO) << "Error " << (i + 1) << ":";
    XLOG(INFO) << "  Test: " << error.testName;
    XLOG(INFO) << "  Device: " << error.deviceName;
    XLOG(INFO) << "  Reason: "
               << (error.reason.empty() ? "<no reason provided>"
                                        : error.reason);
  }
  XLOG(INFO) << "=== END ERRORS SUMMARY ===";
}

} // namespace facebook::fboss::platform::bsp_tests::cpp
