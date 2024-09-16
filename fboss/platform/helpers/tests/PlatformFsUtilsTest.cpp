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
  auto tmpDir = folly::test::TemporaryDirectory();
  fs::path tmpPath{tmpDir.path().string()};
  EXPECT_TRUE(PlatformFsUtils().createDirectories(tmpPath));
  EXPECT_TRUE(fs::exists(tmpPath));
  tmpPath = tmpPath / "a";
  EXPECT_TRUE(PlatformFsUtils().createDirectories(tmpPath));
  EXPECT_TRUE(fs::exists(tmpPath));
  tmpPath = tmpPath / "b" / "c";
  EXPECT_TRUE(PlatformFsUtils().createDirectories(tmpPath));
  EXPECT_TRUE(fs::exists(tmpPath));
  EXPECT_TRUE(PlatformFsUtils().createDirectories("/"));
  EXPECT_FALSE(PlatformFsUtils().createDirectories(""));
}

TEST(PlatformFsUtilsTest, TemporaryRoot) {
  // Folly returns a boost path. Convert to a std::filesystem::path.
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};
  // Test intermixing of std::string, and the usage of absolute paths which are
  // treated as relative to rootDir.
  EXPECT_TRUE(utils.createDirectories("/a/b/c"));

  fs::path tmpPathAbsolute = tmpDirPath / "a" / "b" / "c";
  EXPECT_TRUE(fs::exists(tmpPathAbsolute));

  // Note: This is only "relative" in the sense that it is meant to be relative
  // to the rootDir; as a standalone path it is absolute.
  std::string tmpFilePathRelative = "/a/b/c/d";
  fs::path tmpFilePathAbsolute = tmpPathAbsolute / "d";
  auto content = "text";
  EXPECT_TRUE(utils.writeStringToFile(content, tmpFilePathRelative));
  auto readContent = utils.getStringFileContent(tmpFilePathRelative);
  EXPECT_TRUE(readContent.has_value());
  EXPECT_EQ(readContent.value(), content);
  // Read should produce same results as when using PlatformUtils with no
  // rootDir parameter and the complete absolute path.
  EXPECT_EQ(
      utils.getStringFileContent(tmpFilePathRelative),
      PlatformFsUtils().getStringFileContent(tmpFilePathAbsolute));

  // Redo the above flow, but using default PlatformUtils & absolute paths.
  EXPECT_TRUE(fs::remove(tmpFilePathAbsolute));
  EXPECT_FALSE(fs::exists(tmpFilePathAbsolute));
  EXPECT_TRUE(
      PlatformFsUtils().writeStringToFile(content, tmpFilePathAbsolute));
  readContent = PlatformFsUtils().getStringFileContent(tmpFilePathAbsolute);
  EXPECT_TRUE(readContent.has_value());
  EXPECT_EQ(readContent.value(), content);
  EXPECT_EQ(
      utils.getStringFileContent(tmpFilePathRelative), readContent.value());
}

} // namespace facebook::fboss::platform
