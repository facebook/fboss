// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/helpers/PlatformFsUtils.h"

#include <filesystem>

#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

namespace fs = std::filesystem;
using namespace ::testing;
using namespace facebook::fboss::platform;

namespace facebook::fboss::platform {

TEST(PlatformFsUtilsTest, CreateParentDirectories) {
  auto tmpPath = folly::test::TemporaryDirectory().path();
  EXPECT_TRUE(PlatformFsUtils().createDirectories(tmpPath.string()));
  EXPECT_TRUE(fs::exists(tmpPath.string()));
  tmpPath = tmpPath / "a";
  EXPECT_TRUE(PlatformFsUtils().createDirectories(tmpPath.string()));
  EXPECT_TRUE(fs::exists(tmpPath.string()));
  tmpPath = tmpPath / "b" / "c";
  EXPECT_TRUE(PlatformFsUtils().createDirectories(tmpPath.string()));
  EXPECT_TRUE(fs::exists(tmpPath.string()));
  EXPECT_TRUE(PlatformFsUtils().createDirectories("/"));
  EXPECT_FALSE(PlatformFsUtils().createDirectories(""));
}

} // namespace facebook::fboss::platform
