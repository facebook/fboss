// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/fw_util_helpers.h"

#include <gtest/gtest.h>

using namespace facebook::fboss::platform::fw_util;

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
