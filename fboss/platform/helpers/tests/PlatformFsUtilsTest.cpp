// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/helpers/PlatformFsUtils.h"

#include <filesystem>

#include <folly/FileUtil.h>
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

TEST(PlatformFsUtilsTest, BasicWriteReadTest) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};
  EXPECT_TRUE(utils.writeStringToFile("test", "/a/b/c/d"));
  EXPECT_EQ(utils.getStringFileContent("/a/b/c/d"), "test");
  EXPECT_TRUE(utils.writeStringToFile("overwrite", "/a/b/c/d"));
  EXPECT_EQ(utils.getStringFileContent("/a/b/c/d"), "overwrite");
  // Testing atomic writes here as basically just a sanity check. We rely on
  // folly and their testing to maintain their atomicity guarantees.
  EXPECT_TRUE(utils.writeStringToFile("test2", "/e/f/g/h", true));
  EXPECT_EQ(utils.getStringFileContent("/e/f/g/h"), "test2");
}

TEST(PlatformFsUtilsTest, ExistsFileTest) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};

  // File doesn't exist initially
  EXPECT_FALSE(utils.exists("/test_file.txt"));

  // Create file and verify it exists
  EXPECT_TRUE(utils.writeStringToFile("content", "/test_file.txt"));
  EXPECT_TRUE(utils.exists("/test_file.txt"));
}

TEST(PlatformFsUtilsTest, ExistsDirectoryTest) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};

  // Directory doesn't exist initially
  EXPECT_FALSE(utils.exists("/test_dir"));

  // Create directory and verify it exists
  EXPECT_TRUE(utils.createDirectories("/test_dir"));
  EXPECT_TRUE(utils.exists("/test_dir"));
}

TEST(PlatformFsUtilsTest, ExistsWithRootDir) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};
  PlatformFsUtils noRootUtils;

  // Create file with rootDir utils
  EXPECT_TRUE(utils.writeStringToFile("test", "/exists_test.txt"));

  // Verify with rootDir utils
  EXPECT_TRUE(utils.exists("/exists_test.txt"));

  // Verify with absolute path using no-root utils
  fs::path absolutePath = tmpDirPath / "exists_test.txt";
  EXPECT_TRUE(noRootUtils.exists(absolutePath));
}

TEST(PlatformFsUtilsTest, LsEmptyDirectory) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};

  EXPECT_TRUE(utils.createDirectories("/empty_dir"));
  auto it = utils.ls("/empty_dir");
  EXPECT_EQ(it, fs::directory_iterator{});
}

TEST(PlatformFsUtilsTest, LsWithFiles) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};

  // Create directory with files
  EXPECT_TRUE(utils.writeStringToFile("file1", "/ls_test/file1.txt"));
  EXPECT_TRUE(utils.writeStringToFile("file2", "/ls_test/file2.txt"));
  EXPECT_TRUE(utils.writeStringToFile("file3", "/ls_test/file3.txt"));

  // List directory
  auto it = utils.ls("/ls_test");
  std::vector<std::string> files;
  for (const auto& entry : it) {
    files.push_back(entry.path().filename().string());
  }

  EXPECT_EQ(files.size(), 3);
  EXPECT_TRUE(
      std::find(files.begin(), files.end(), "file1.txt") != files.end());
  EXPECT_TRUE(
      std::find(files.begin(), files.end(), "file2.txt") != files.end());
  EXPECT_TRUE(
      std::find(files.begin(), files.end(), "file3.txt") != files.end());
}

TEST(PlatformFsUtilsTest, LsWithSubdirectories) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};

  // Create directory with subdirectories and files
  EXPECT_TRUE(utils.createDirectories("/ls_mixed/subdir1"));
  EXPECT_TRUE(utils.createDirectories("/ls_mixed/subdir2"));
  EXPECT_TRUE(utils.writeStringToFile("file", "/ls_mixed/file.txt"));

  // List directory
  auto it = utils.ls("/ls_mixed");
  std::vector<std::string> entries;
  for (const auto& entry : it) {
    entries.push_back(entry.path().filename().string());
  }

  EXPECT_EQ(entries.size(), 3);
  EXPECT_TRUE(
      std::find(entries.begin(), entries.end(), "subdir1") != entries.end());
  EXPECT_TRUE(
      std::find(entries.begin(), entries.end(), "subdir2") != entries.end());
  EXPECT_TRUE(
      std::find(entries.begin(), entries.end(), "file.txt") != entries.end());
}

TEST(PlatformFsUtilsTest, WriteStringToSysfs) {
  // Since we can't easily test real sysfs, we'll test with a regular temp file
  // to verify the behavior (newline appending)
  auto tmpDir = folly::test::TemporaryDirectory();
  fs::path tmpFilePath = fs::path(tmpDir.path().string()) / "sysfs_test.txt";
  PlatformFsUtils utils;

  // Create the file first (sysfs files already exist in real systems)
  EXPECT_TRUE(folly::writeFile(std::string("initial"), tmpFilePath.c_str()));

  // Write using writeStringToSysfs
  EXPECT_TRUE(utils.writeStringToSysfs("test_value", tmpFilePath));

  // Read back and verify newline was appended
  std::string content;
  EXPECT_TRUE(folly::readFile(tmpFilePath.c_str(), content));
  EXPECT_EQ(content, "test_value\n");
}

TEST(PlatformFsUtilsTest, WriteStringToSysfsNewlineAppended) {
  auto tmpDir = folly::test::TemporaryDirectory();
  fs::path tmpFilePath = fs::path(tmpDir.path().string()) / "sysfs_newline.txt";
  PlatformFsUtils utils;

  // Create the file first (sysfs files already exist in real systems)
  EXPECT_TRUE(folly::writeFile(std::string("initial"), tmpFilePath.c_str()));

  // Write content that already has a newline
  EXPECT_TRUE(utils.writeStringToSysfs("value_with_newline\n", tmpFilePath));

  // Verify an additional newline is appended
  std::string content;
  EXPECT_TRUE(folly::readFile(tmpFilePath.c_str(), content));
  EXPECT_EQ(content, "value_with_newline\n\n");
}

TEST(PlatformFsUtilsTest, GetStringFileContentNonExistent) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};

  // Try to read non-existent file
  auto content = utils.getStringFileContent("/non_existent_file.txt");
  EXPECT_FALSE(content.has_value());
}

TEST(PlatformFsUtilsTest, GetStringFileContentEmptyFile) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};

  // Create empty file
  EXPECT_TRUE(utils.writeStringToFile("", "/empty_file.txt"));

  // Read empty file
  auto content = utils.getStringFileContent("/empty_file.txt");
  EXPECT_TRUE(content.has_value());
  EXPECT_EQ(content.value(), "");
}

TEST(PlatformFsUtilsTest, CreateDirectoriesWithInvalidPath) {
  PlatformFsUtils utils;

  EXPECT_FALSE(utils.createDirectories("/proc/invalid_dir_test/subdir"));
}

TEST(PlatformFsUtilsTest, WriteStringToFileFailsWhenParentCreationFails) {
  PlatformFsUtils utils;

  EXPECT_FALSE(
      utils.writeStringToFile("content", "/proc/invalid/test/file.txt"));
}

TEST(PlatformFsUtilsTest, WriteStringToFileNonAtomicWriteFailure) {
  PlatformFsUtils utils;

  auto tmpDir = folly::test::TemporaryDirectory();
  fs::path dirPath = fs::path(tmpDir.path().string()) / "test_dir";
  fs::create_directory(dirPath);

  // Trying to write to a directory should fail
  EXPECT_FALSE(utils.writeStringToFile("content", dirPath, false));
}

TEST(PlatformFsUtilsTest, WriteStringToFileAtomicWriteFailure) {
  PlatformFsUtils utils;

  // Create a directory, then try to atomically write to it as if it were a file
  auto tmpDir = folly::test::TemporaryDirectory();
  fs::path dirPath = fs::path(tmpDir.path().string()) / "test_dir_atomic";
  fs::create_directory(dirPath);

  // Trying to atomically write to a directory should fail
  EXPECT_FALSE(utils.writeStringToFile("content", dirPath, true));
}

TEST(PlatformFsUtilsTest, WriteStringToSysfsFailure) {
  PlatformFsUtils utils;

  EXPECT_FALSE(
      utils.writeStringToSysfs("test", "/nonexistent/path/to/sysfs/file"));
}

TEST(PlatformFsUtilsTest, GetStringFileContentFailure) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};

  // Create a directory and try to read it as a file
  EXPECT_TRUE(utils.createDirectories("/test_dir_read"));

  // Trying to read a directory as a file should fail
  auto content = utils.getStringFileContent("/test_dir_read");
  EXPECT_FALSE(content.has_value());
}

TEST(PlatformFsUtilsTest, ConcatWithEmptyRootDir) {
  // Test concat behavior when rootDir is empty
  // This is handled by the PlatformFsUtils constructor when no rootDir is
  // provided
  PlatformFsUtils utils; // No rootDir specified

  auto tmpDir = folly::test::TemporaryDirectory();
  fs::path tmpFilePath =
      fs::path(tmpDir.path().string()) / "test_empty_root.txt";

  // Write and read should work with absolute paths when rootDir is empty
  EXPECT_TRUE(utils.writeStringToFile("content", tmpFilePath));
  EXPECT_TRUE(utils.exists(tmpFilePath));
  auto content = utils.getStringFileContent(tmpFilePath);
  EXPECT_TRUE(content.has_value());
  EXPECT_EQ(content.value(), "content");
}

TEST(PlatformFsUtilsTest, WriteStringToFileWithCustomWriteFlags) {
  fs::path tmpDirPath =
      fs::path(folly::test::TemporaryDirectory().path().string());
  PlatformFsUtils utils{tmpDirPath};

  // Write initial content with O_WRONLY | O_CREAT | O_TRUNC
  EXPECT_TRUE(utils.writeStringToFile(
      "initial", "/flags_test.txt", false, O_WRONLY | O_CREAT | O_TRUNC));

  // Verify content
  auto content = utils.getStringFileContent("/flags_test.txt");
  EXPECT_TRUE(content.has_value());
  EXPECT_EQ(content.value(), "initial");

  // Overwrite with different flags
  EXPECT_TRUE(utils.writeStringToFile(
      "replaced", "/flags_test.txt", false, O_WRONLY | O_TRUNC));

  content = utils.getStringFileContent("/flags_test.txt");
  EXPECT_TRUE(content.has_value());
  EXPECT_EQ(content.value(), "replaced");
}

} // namespace facebook::fboss::platform
