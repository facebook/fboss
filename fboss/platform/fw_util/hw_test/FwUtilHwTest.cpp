// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/hw_test/FwUtilHwTest.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/fw_util/ConfigValidator.h"
#include "fboss/platform/helpers/Init.h"

namespace facebook::fboss::platform::fw_util {

FwUtilHwTest::~FwUtilHwTest() {}

void FwUtilHwTest::SetUp() {
  // TODO: Add arguments for testing
  fwUtilImpl_ = std::make_unique<FwUtilImpl>("", "", true, true);

  auto fwUtilConfJson = ConfigLib().getFwUtilConfig();
  try {
    apache::thrift::SimpleJSONSerializer::deserialize<FwUtilConfig>(
        fwUtilConfJson, fwUtilConfig_);
  } catch (const std::exception& e) {
    throw std::runtime_error(
        fmt::format(
            "fw_util_hw_test failed to deserialize config: {}", e.what()));
  }
}

TEST_F(FwUtilHwTest, validateConfig) {
  EXPECT_TRUE(ConfigValidator().isValid(fwUtilConfig_));
}

TEST_F(FwUtilHwTest, GetAllFirmwareVersionInfo) {
  EXPECT_NO_THROW(fwUtilImpl_->printVersion("all"));
}

TEST_F(FwUtilHwTest, GetEachFirmwareVersionInfo) {
  auto fwNameList = fwUtilImpl_->getFpdNameList();
  for (const auto& fwName : fwNameList) {
    EXPECT_NO_THROW(fwUtilImpl_->printVersion(fwName));
  }
}

TEST_F(FwUtilHwTest, listFirmwareNames) {
  auto fwNameList = fwUtilImpl_->getFpdNameList();

  EXPECT_GT(fwNameList.size(), 0);
  for (const auto& fwName : fwNameList) {
    // FW name should not be empty
    EXPECT_TRUE(fwName.length() > 0);
  }
}

} // namespace facebook::fboss::platform::fw_util

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::fboss::platform::helpers::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
