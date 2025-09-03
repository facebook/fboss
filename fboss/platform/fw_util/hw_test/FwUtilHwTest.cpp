// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/hw_test/FwUtilHwTest.h"

#include <gtest/gtest.h>

#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/helpers/Init.h"

namespace facebook::fboss::platform::fw_util {

FwUtilHwTest::~FwUtilHwTest() {}

void FwUtilHwTest::SetUp() {
  // TODO: Add arguments for testing
  fwUtilImpl_ = std::make_unique<FwUtilImpl>("", "", true, true);
}

TEST_F(FwUtilHwTest, ListFirmwareDevices) {
  EXPECT_GT(fwUtilImpl_->printFpdList().size(), 0);
}
TEST_F(FwUtilHwTest, GetAllFirmwareVersionInfo) {
  EXPECT_NO_THROW(fwUtilImpl_->printVersion("all"));
}

TEST_F(FwUtilHwTest, GetEachFirmwareVersionInfo) {
  auto fwList = fwUtilImpl_->printFpdList();
  std::istringstream iss(fwList);
  std::string fwNames;
  while (iss >> fwNames) {
    EXPECT_NO_THROW(fwUtilImpl_->printVersion(fwNames));
  }
}

} // namespace facebook::fboss::platform::fw_util

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
