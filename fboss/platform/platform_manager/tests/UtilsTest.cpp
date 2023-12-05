// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>
#include <stdexcept>

#include <folly/experimental/TestUtil.h>
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

TEST(UtilsTest, CreateParentDirectories) {
  auto tmpPath = folly::test::TemporaryDirectory().path();
  EXPECT_TRUE(Utils().createDirectories(tmpPath.string()));
  EXPECT_TRUE(fs::exists(tmpPath.string()));
  tmpPath = tmpPath / "a";
  EXPECT_TRUE(Utils().createDirectories(tmpPath.string()));
  EXPECT_TRUE(fs::exists(tmpPath.string()));
  tmpPath = tmpPath / "b" / "c";
  EXPECT_TRUE(Utils().createDirectories(tmpPath.string()));
  EXPECT_TRUE(fs::exists(tmpPath.string()));
  EXPECT_TRUE(Utils().createDirectories("/"));
  EXPECT_FALSE(Utils().createDirectories(""));
}

TEST(UtilsTest, ParseDevicePath) {
  EXPECT_EQ(
      makeDevicePathPair("/", "IDPROM"), Utils().parseDevicePath("/[IDPROM]"));
  EXPECT_EQ(
      makeDevicePathPair("/MCB_SLOT@0", "sensor"),
      Utils().parseDevicePath("/MCB_SLOT@0/[sensor]"));
  EXPECT_EQ(
      makeDevicePathPair("/MCB_SLOT@0/SMB_SLOT@11", "SMB_IOB_I2C_1"),
      Utils().parseDevicePath("/MCB_SLOT@0/SMB_SLOT@11/[SMB_IOB_I2C_1]"));
  EXPECT_THROW(Utils().parseDevicePath("ABCDE/[abc]"), std::runtime_error);
  EXPECT_THROW(Utils().parseDevicePath("/MCB_SLOT/[abc]"), std::runtime_error);
  EXPECT_THROW(Utils().parseDevicePath("/MCB_SLOT@1/[]"), std::runtime_error);
}
