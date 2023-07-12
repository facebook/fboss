// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/lib/CommonFileUtils.h"
#include <boost/filesystem/operations.hpp>
#include <folly/experimental/TestUtil.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include "fboss/agent/SysError.h"

#include <fcntl.h>

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

} // namespace facebook::fboss
