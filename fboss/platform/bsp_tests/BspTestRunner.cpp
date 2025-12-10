// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "fboss/platform/bsp_tests/BspTestEnvironment.h"

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/PlatformNameLib.h"

DEFINE_bool(
    enable_stress_tests,
    false,
    "Enable stress tests that may take a long time to run");
DEFINE_string(
    bsp_tests_config_file,
    "",
    "[OSS-Only] Path to bsp_tests.json file.");
DEFINE_string(
    platform_manager_config_file,
    "",
    "[OSS-Only] Path to platform_manager.json file.");

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;
using namespace facebook::fboss::platform::bsp_tests;

// Custom test filter to exclude stress tests unless enabled
class StressTestFilter : public ::testing::EmptyTestEventListener {
 public:
  void OnTestStart(const ::testing::TestInfo& test_info) override {
    // Skip stress tests if not enabled
    if (!FLAGS_enable_stress_tests &&
        std::string(test_info.test_suite_name()).find("Stress") !=
            std::string::npos) {
      GTEST_SKIP() << "Skipping stress test. Use --enable_stress_tests to run.";
    }
  }
};

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  helpers::init(&argc, &argv);

  auto platformName =
      helpers::PlatformNameLib().getPlatformName().value_or("unknown");

  XLOG(INFO) << "Starting BSP tests";
  XLOG(INFO) << "Platform: " << platformName;

  ::testing::UnitTest::GetInstance()->listeners().Append(new StressTestFilter);

  std::string jsonBspTestsConfig;
  if (FLAGS_bsp_tests_config_file.empty()) {
    jsonBspTestsConfig = ConfigLib().getBspTestConfig();
  } else {
    folly::readFile(FLAGS_bsp_tests_config_file.c_str(), jsonBspTestsConfig);
  }
  std::string jsonPMConfig;
  if (FLAGS_platform_manager_config_file.empty()) {
    jsonPMConfig = ConfigLib().getPlatformManagerConfig();
  } else {
    folly::readFile(FLAGS_platform_manager_config_file.c_str(), jsonPMConfig);
  }

  auto env = BspTestEnvironment::GetInstance(
      platformName, jsonBspTestsConfig, jsonPMConfig);

  auto ret = RUN_ALL_TESTS();
  env->printAllRecordedErrors();
  return ret;
}
