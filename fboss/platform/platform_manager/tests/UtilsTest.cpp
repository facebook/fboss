// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/Utils.h"

namespace fs = std::filesystem;
using namespace ::testing;
using namespace facebook::fboss::platform::platform_manager;

namespace {
std::pair<std::string, std::string> makeDevicePathPair(
    std::string slotPath,
    std::string deviceName) {
  return std::make_pair(slotPath, deviceName);
}
}; // namespace

TEST(UtilsTest, ParseDevicePath) {
  EXPECT_EQ(
      makeDevicePathPair("/", "IDPROM"), Utils().parseDevicePath("/[IDPROM]"));
  EXPECT_EQ(
      makeDevicePathPair("/MCB_SLOT@0", "sensor"),
      Utils().parseDevicePath("/MCB_SLOT@0/[sensor]"));
  EXPECT_EQ(
      makeDevicePathPair("/MCB_SLOT@0/SMB_SLOT@11", "SMB_IOB_I2C_1"),
      Utils().parseDevicePath("/MCB_SLOT@0/SMB_SLOT@11/[SMB_IOB_I2C_1]"));
  EXPECT_NO_THROW(Utils().parseDevicePath("ABCDE/[abc]"));
  EXPECT_NO_THROW(Utils().parseDevicePath("/MCB_SLOT/[abc]"));
  EXPECT_NO_THROW(Utils().parseDevicePath("/MCB_SLOT@1/[]"));
}
