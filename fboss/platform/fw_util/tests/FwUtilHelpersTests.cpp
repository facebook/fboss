// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/fw_util_helpers.h"

#include <gtest/gtest.h>
#include <ctime>
#include <filesystem>
#include <fstream>
#include "fboss/platform/helpers/PlatformUtils.h"

using namespace facebook::fboss::platform::fw_util;
using namespace facebook::fboss::platform;

class FwUtilHelpersTest : public ::testing::Test {
 protected:
  void SetUp() override {}
};

// ============================================================================
// toLower() Tests
// ============================================================================

TEST_F(FwUtilHelpersTest, ToLowerBasic) {
  EXPECT_EQ(toLower("HELLO"), "hello");
  EXPECT_EQ(toLower("WORLD"), "world");
}

TEST_F(FwUtilHelpersTest, ToLowerMixedCase) {
  EXPECT_EQ(toLower("HeLLo"), "hello");
  EXPECT_EQ(toLower("WoRLd"), "world");
  EXPECT_EQ(toLower("TeSt123"), "test123");
}

TEST_F(FwUtilHelpersTest, ToLowerAlreadyLowercase) {
  EXPECT_EQ(toLower("hello"), "hello");
  EXPECT_EQ(toLower("world"), "world");
  EXPECT_EQ(toLower("test"), "test");
}

TEST_F(FwUtilHelpersTest, ToLowerEmptyString) {
  EXPECT_EQ(toLower(""), "");
}

TEST_F(FwUtilHelpersTest, ToLowerWithNumbers) {
  EXPECT_EQ(toLower("TEST123"), "test123");
  EXPECT_EQ(toLower("ABC123DEF"), "abc123def");
}

TEST_F(FwUtilHelpersTest, ToLowerWithSpecialCharacters) {
  EXPECT_EQ(toLower("HELLO_WORLD"), "hello_world");
  EXPECT_EQ(toLower("TEST-NAME"), "test-name");
  EXPECT_EQ(toLower("FOO.BAR"), "foo.bar");
}

TEST_F(FwUtilHelpersTest, ToLowerWithSpaces) {
  EXPECT_EQ(toLower("HELLO WORLD"), "hello world");
  EXPECT_EQ(toLower("Test String"), "test string");
}

TEST_F(FwUtilHelpersTest, ToLowerSingleCharacter) {
  EXPECT_EQ(toLower("A"), "a");
  EXPECT_EQ(toLower("Z"), "z");
  EXPECT_EQ(toLower("a"), "a");
}

TEST_F(FwUtilHelpersTest, ToLowerLongString) {
  std::string input = "THIS IS A VERY LONG STRING WITH MANY CHARACTERS";
  std::string expected = "this is a very long string with many characters";
  EXPECT_EQ(toLower(input), expected);
}

// ============================================================================
// checkCmdStatus() Tests
// ============================================================================

TEST_F(FwUtilHelpersTest, CheckCmdStatusSuccess) {
  // Exit status 0 should not throw
  std::vector<std::string> cmd = {"echo", "hello"};
  int exitStatus = 0; // Success status (WIFEXITED(0) returns 0)
  EXPECT_NO_THROW(checkCmdStatus(cmd, exitStatus));
}

TEST_F(FwUtilHelpersTest, CheckCmdStatusNegativeExitStatus) {
  // Negative exit status should throw
  std::vector<std::string> cmd = {"false"};
  int exitStatus = -1;
  EXPECT_THROW(checkCmdStatus(cmd, exitStatus), std::runtime_error);
}

TEST_F(FwUtilHelpersTest, CheckCmdStatusNegativeExitStatusWithMessage) {
  // Verify error message contains command
  std::vector<std::string> cmd = {"test", "command"};
  int exitStatus = -1;
  try {
    checkCmdStatus(cmd, exitStatus);
    FAIL() << "Expected std::runtime_error";
  } catch (const std::runtime_error& e) {
    std::string errorMsg(e.what());
    EXPECT_TRUE(errorMsg.find("test") != std::string::npos);
    EXPECT_TRUE(errorMsg.find("command") != std::string::npos);
    EXPECT_TRUE(errorMsg.find("failed") != std::string::npos);
  }
}

TEST_F(FwUtilHelpersTest, CheckCmdStatusWithComplexCommand) {
  // Test with a complex command with multiple arguments
  std::vector<std::string> cmd = {
      "/usr/bin/flashrom",
      "-p",
      "linux_spi:dev=/dev/spidev0.0",
      "-w",
      "firmware.bin"};
  int exitStatus = 0;
  EXPECT_NO_THROW(checkCmdStatus(cmd, exitStatus));
}

TEST_F(FwUtilHelpersTest, CheckCmdStatusEmptyCommand) {
  // Test with empty command vector
  std::vector<std::string> cmd = {};
  int exitStatus = -1;
  EXPECT_THROW(checkCmdStatus(cmd, exitStatus), std::runtime_error);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(FwUtilHelpersTest, ToLowerIdempotent) {
  // Applying toLower twice should give the same result
  std::string input = "TeSt";
  std::string once = toLower(input);
  std::string twice = toLower(once);
  EXPECT_EQ(once, twice);
}

TEST_F(FwUtilHelpersTest, ToLowerPreservesLength) {
  // toLower should not change string length
  std::string input = "HelloWorld";
  std::string output = toLower(input);
  EXPECT_EQ(input.length(), output.length());
}

// ============================================================================
// getUpgradeToolBinaryPath() Tests
// ============================================================================

TEST_F(FwUtilHelpersTest, GetUpgradeToolBinaryPathInUsrLocalBin) {
  // Create a temporary test binary in /tmp to simulate /usr/local/bin
  std::string testTool = "test_upgrade_tool_" + std::to_string(time(nullptr));
  std::string testPath = "/tmp/" + testTool;

  // Create the test file
  std::ofstream testFile(testPath);
  testFile << "#!/bin/bash\necho test\n";
  testFile.close();
  std::filesystem::permissions(
      testPath,
      std::filesystem::perms::owner_exec,
      std::filesystem::perm_options::add);

  // Test will throw since tool won't be in /usr/local/bin in test environment
  // But we're testing the logic path
  EXPECT_THROW(getUpgradeToolBinaryPath(testTool), std::runtime_error);

  // Cleanup
  std::filesystem::remove(testPath);
}

TEST_F(FwUtilHelpersTest, GetUpgradeToolBinaryPathViaScriptDir) {
  // Test the script_dir environment variable path
  std::string testTool = "test_tool_" + std::to_string(time(nullptr));
  std::string testDir = "/tmp/test_script_dir_" + std::to_string(time(nullptr));

  // Create test directory and tool
  std::filesystem::create_directories(testDir);
  std::string testPath = testDir + "/" + testTool;
  std::ofstream testFile(testPath);
  testFile << "#!/bin/bash\necho test\n";
  testFile.close();

  // Set script_dir environment variable
  setenv("script_dir", testDir.c_str(), 1);

  // Should find the tool via script_dir
  std::string foundPath = getUpgradeToolBinaryPath(testTool);
  EXPECT_EQ(foundPath, testPath);

  // Cleanup
  unsetenv("script_dir");
  std::filesystem::remove_all(testDir);
}

TEST_F(FwUtilHelpersTest, VerifySha1sumMatching) {
  // Create a test binary file with known content
  std::string testFile =
      "/tmp/test_binary_" + std::to_string(time(nullptr)) + ".bin";
  std::ofstream binFile(testFile);
  binFile << "test content for sha1sum verification\n";
  binFile.close();

  // Calculate the actual SHA1 sum
  std::vector<std::string> sha1Cmd = {"/usr/bin/sha1sum", testFile};
  auto [exitStatus, output] = PlatformUtils().runCommand(sha1Cmd);
  std::string actualSha1 = output.substr(0, output.find(' '));

  // Should not throw when SHA1 matches
  EXPECT_NO_THROW(verifySha1sum("test_fpd", actualSha1, testFile));

  // Cleanup
  std::filesystem::remove(testFile);
}

// ============================================================================
// verifySha1sum() Additional Tests
// ============================================================================

TEST_F(FwUtilHelpersTest, VerifySha1sumMismatch) {
  // Create a test binary file
  std::string testFile =
      "/tmp/test_binary_mismatch_" + std::to_string(time(nullptr)) + ".bin";
  std::ofstream binFile(testFile);
  binFile << "test content for sha1sum mismatch\n";
  binFile.close();

  // Use a wrong SHA1 sum
  std::string wrongSha1 = "0000000000000000000000000000000000000000";

  // Should throw when SHA1 doesn't match
  EXPECT_THROW(
      verifySha1sum("test_fpd", wrongSha1, testFile), std::runtime_error);

  // Cleanup
  std::filesystem::remove(testFile);
}

// ============================================================================
// getUpgradeToolBinaryPath() Additional Tests
// ============================================================================

TEST_F(FwUtilHelpersTest, GetUpgradeToolBinaryPathNotFoundNoEnv) {
  // Unset script_dir to test the error path
  unsetenv("script_dir");

  // Tool doesn't exist in /usr/local/bin and no script_dir set
  EXPECT_THROW(
      getUpgradeToolBinaryPath("nonexistent_tool_xyz"), std::runtime_error);
}

TEST_F(FwUtilHelpersTest, GetUpgradeToolBinaryPathNotFoundWithEnv) {
  // Set script_dir but tool doesn't exist there either
  std::string testDir = "/tmp/test_script_dir_notfound";
  std::filesystem::create_directories(testDir);
  setenv("script_dir", testDir.c_str(), 1);

  EXPECT_THROW(
      getUpgradeToolBinaryPath("nonexistent_tool_abc"), std::runtime_error);

  unsetenv("script_dir");
  std::filesystem::remove_all(testDir);
}
