// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "fboss/qsfp_service/module/QsfpModule.h"
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

  QsfpModule* getModule(int tcvrId) const;
  BspTransceiverImpl* getImpl(int tcvrId) const;
  const HalTestConfig& getConfig() const;
  std::vector<int> getPresentTransceiverIds() const;

 private:
  static inline HalTestConfig config_;
  static inline std::map<int, hal_test::HalTestModule> modules_;
};

} // namespace facebook::fboss
