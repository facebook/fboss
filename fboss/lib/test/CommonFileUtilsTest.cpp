// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/lib/CommonFileUtils.h"
#include <folly/logging/xlog.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

TEST(CommonFileUtils, CreateRemoveFile) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto testfile1 = tmpDir.path().string() + "testfile_1";
  EXPECT_NO_THROW(createFile(testfile1));
  CHECK(checkFileExists(testfile1));

  removeFile(testfile1, false);
  CHECK(!checkFileExists(testfile1));
}

TEST(CommonFileUtils, WriteReadFile) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto testfile2 = tmpDir.path().string() + "testfile_2";

  EXPECT_NO_THROW(createFile(testfile2));
  CHECK(checkFileExists(testfile2));

  auto inputString = "test string";
  writeSysfs(testfile2, inputString);
  auto fileContent = readSysfs(testfile2);
  EXPECT_EQ(fileContent, inputString);

  removeFile(testfile2, false);
  CHECK(!checkFileExists(testfile2));
}

TEST(CommonFileUtils, CreateSymLink) {
  auto tmpDir = folly::test::TemporaryDirectory();
  auto testfile1 = tmpDir.path().string() + "testfile_1";
  auto testfile2 = tmpDir.path().string() + "testfile_2";
  createFile(testfile1);
  createFile(testfile2);

  createSymLink(testfile1, testfile2);
  ASSERT_TRUE(std::filesystem::exists(testfile1));
  ASSERT_TRUE(std::filesystem::is_symlink(testfile1));
  ASSERT_EQ(std::filesystem::read_symlink(testfile1), testfile2);
}
} // namespace facebook::fboss
