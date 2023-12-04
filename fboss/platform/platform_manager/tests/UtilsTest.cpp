// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>

#include <folly/experimental/TestUtil.h>
#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/Utils.h"

namespace fs = std::filesystem;
using namespace ::testing;
using namespace facebook::fboss::platform::platform_manager;

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
