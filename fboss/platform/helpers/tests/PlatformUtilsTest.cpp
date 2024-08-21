// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>

#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/helpers/PlatformUtils.h"

namespace fs = std::filesystem;
using namespace ::testing;
using namespace facebook::fboss::platform;

namespace facebook::fboss::platform {

TEST(PlatformUtilsTest, CreateParentDirectories) {
  auto tmpPath = folly::test::TemporaryDirectory().path();
  EXPECT_TRUE(PlatformUtils().createDirectories(tmpPath.string()));
  EXPECT_TRUE(fs::exists(tmpPath.string()));
  tmpPath = tmpPath / "a";
  EXPECT_TRUE(PlatformUtils().createDirectories(tmpPath.string()));
  EXPECT_TRUE(fs::exists(tmpPath.string()));
  tmpPath = tmpPath / "b" / "c";
  EXPECT_TRUE(PlatformUtils().createDirectories(tmpPath.string()));
  EXPECT_TRUE(fs::exists(tmpPath.string()));
  EXPECT_TRUE(PlatformUtils().createDirectories("/"));
  EXPECT_FALSE(PlatformUtils().createDirectories(""));
}

} // namespace facebook::fboss::platform
