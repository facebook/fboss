// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <vector>

#include "fboss/platform/bsp_tests/cpp/BspTest.h"
#include "fboss/platform/bsp_tests/cpp/utils/KmodUtils.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

using platform_manager::BspKmodsFile;

// Test fixture for kmod tests
class KmodTest : public BspTest {
 protected:
  void SetUp() override {
    // Ensure we have a clean environment before each test
    KmodUtils::unloadKmods(*GetRuntimeConfig().kmods());
  }

  void TearDown() override {
    // Clean up after each test
    KmodUtils::unloadKmods(*GetRuntimeConfig().kmods());
  }
};

// Test loading kernel modules
TEST_F(KmodTest, LoadKmods) {
  const auto& kmods = *GetRuntimeConfig().kmods();
  KmodUtils::loadKmods(kmods);

  // Verify modules are loaded
  auto loadedKmods = KmodUtils::getLoadedKmods(kmods);
  EXPECT_FALSE(loadedKmods.empty()) << "No kernel modules were loaded";

  // Check that all expected modules are loaded
  size_t expectedCount = kmods.bspKmods()->size() + kmods.sharedKmods()->size();
  EXPECT_EQ(loadedKmods.size(), expectedCount)
      << "Not all expected kernel modules were loaded";
}

// Test unloading kernel modules
TEST_F(KmodTest, UnloadKmods) {
  const auto& kmods = *GetRuntimeConfig().kmods();

  KmodUtils::loadKmods(kmods);
  KmodUtils::unloadKmods(kmods);

  auto loadedKmods = KmodUtils::getLoadedKmods(kmods);
  EXPECT_TRUE(loadedKmods.empty()) << "Some kernel modules were not unloaded";
}

// Test fixture for kmod stress tests
class KmodStressTest : public KmodTest {};

// Stress test for loading and unloading modules
TEST_F(KmodStressTest, KmodLoadUnloadStress) {
  const auto& kmods = *GetRuntimeConfig().kmods();

  for (int i = 0; i < 10; i++) {
    XLOG(INFO) << "Stress test iteration " << i + 1;
    KmodUtils::loadKmods(kmods);
    KmodUtils::unloadKmods(kmods);
  }

  auto loadedKmods = KmodUtils::getLoadedKmods(kmods);
  EXPECT_TRUE(loadedKmods.empty())
      << "Some kernel modules were not unloaded after stress test";
}

// Test fbsp-remove script
TEST_F(KmodTest, FbspRemove) {
  const auto& kmods = *GetRuntimeConfig().kmods();

  KmodUtils::loadKmods(kmods);

  try {
    KmodUtils::fbspRemove(GetPlatformManagerConfig());
  } catch (const std::exception& e) {
    FAIL() << "fbsp-remove failed: " << e.what();
  }

  auto loadedKmods = KmodUtils::getLoadedKmods(kmods);
  EXPECT_TRUE(loadedKmods.empty())
      << "Some kernel modules were not unloaded by fbsp-remove. Loaded kmods: "
      << folly::join(", ", loadedKmods);
}

} // namespace facebook::fboss::platform::bsp_tests::cpp
