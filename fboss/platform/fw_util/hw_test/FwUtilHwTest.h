// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gtest/gtest.h>

#include "fboss/platform/fw_util/FwUtilImpl.h"

namespace facebook::fboss::platform::fw_util {

class FwUtilHwTest : public ::testing::Test {
 public:
  ~FwUtilHwTest() override;

  void SetUp() override;

 protected:
  std::unique_ptr<fw_util::FwUtilImpl> fwUtilImpl_;
};

} // namespace facebook::fboss::platform::fw_util
