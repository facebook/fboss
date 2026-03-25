// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/test/hal_test/HalTestParallel.h"
#include "fboss/qsfp_service/test/hal_test/HalTestUtils.h"
#include "fboss/qsfp_service/test/hal_test/gen-cpp2/hal_test_config_types.h"

DECLARE_string(hal_test_config);

namespace facebook::fboss {

class HalTest : public ::testing::Test {
 public:
  static void SetUpTestSuite();
  static void TearDownTestSuite();

 protected:
  void SetUp() override;

  // Override to skip startup firmware upgrades (e.g. in T0HalTest).
  virtual bool shouldApplyStartupFirmware() const {
    return true;
  }

  QsfpModule* getModule(int tcvrId) const;
  BspTransceiverImpl* getImpl(int tcvrId) const;
  const HalTestConfig& getConfig() const;
  std::vector<int> getPresentTransceiverIds() const;

  // Run fn in parallel for each present transceiver. If filter is provided,
  // transceivers for which it returns false are marked as skipped. If ALL
  // transceivers are skipped, the test is skipped via GTEST_SKIP().
  void forEachTransceiverParallel(
      std::function<void(hal_test::TransceiverTestResult&, int)> fn,
      std::function<bool(int)> filter = nullptr);

 private:
  static inline HalTestConfig config_;
  static inline std::map<int, hal_test::HalTestModule> modules_;
};

// T0: runs first (alphabetically before T1). Skips startup firmware upgrades.
class T0HalTest : public HalTest {
 protected:
  bool shouldApplyStartupFirmware() const override {
    return false;
  }
};

// T1: runs after T0. Uses default behavior (applies startup firmware).
class T1HalTest : public HalTest {};

// T2: runs after T1. Same behavior as T1.
class T2HalTest : public HalTest {};

} // namespace facebook::fboss
