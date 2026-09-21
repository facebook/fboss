// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/helpers/PlatformUtils.h"

#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::fboss::platform;

namespace facebook::fboss::platform {

// Test fixture for PlatformUtils tests
class PlatformUtilsTest : public ::testing::Test {
 protected:
  PlatformUtils utils_;
};

// Tests for execCommand method
TEST_F(PlatformUtilsTest, ExecCommandSuccess) {
  auto [exitCode, output] = utils_.execCommand("echo 'hello world'");
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "hello world\n");
}

TEST_F(PlatformUtilsTest, ExecCommandWithExitCode) {
  auto [exitCode, output] = utils_.execCommand("exit 42");
  EXPECT_EQ(exitCode, 42);
}

TEST_F(PlatformUtilsTest, ExecCommandMultiline) {
  auto [exitCode, output] =
      utils_.execCommand("echo 'line1'; echo 'line2'; echo 'line3'");
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "line1\nline2\nline3\n");
}

TEST_F(PlatformUtilsTest, ExecCommandEmptyOutput) {
  auto [exitCode, output] = utils_.execCommand("true");
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "");
}

TEST_F(PlatformUtilsTest, ExecCommandWithPipe) {
  auto [exitCode, output] = utils_.execCommand("echo 'test' | tr 'a-z' 'A-Z'");
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "TEST\n");
}

// Tests for runCommand method
TEST_F(PlatformUtilsTest, RunCommandSuccess) {
  auto [exitCode, output] = utils_.runCommand({"/bin/echo", "hello", "world"});
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "hello world\n");
}

TEST_F(PlatformUtilsTest, RunCommandSingleArg) {
  auto [exitCode, output] = utils_.runCommand({"/bin/echo", "test"});
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "test\n");
}

TEST_F(PlatformUtilsTest, RunCommandNoArgs) {
  auto [exitCode, output] = utils_.runCommand({"/bin/echo"});
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "\n");
}

TEST_F(PlatformUtilsTest, RunCommandWithExitCode) {
  auto [exitCode, output] = utils_.runCommand({"/bin/sh", "-c", "exit 10"});
  EXPECT_EQ(exitCode, 10);
}

TEST_F(PlatformUtilsTest, RunCommandInvalidExecutable) {
  auto [exitCode, output] = utils_.runCommand({"/nonexistent/command", "arg1"});
  EXPECT_EQ(exitCode, -1);
  EXPECT_FALSE(output.empty());
}

// Tests for runCommandWithStdin method
TEST_F(PlatformUtilsTest, RunCommandWithStdinSuccess) {
  auto [exitCode, output] =
      utils_.runCommandWithStdin({"/bin/cat"}, "hello from stdin");
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "hello from stdin");
}

TEST_F(PlatformUtilsTest, RunCommandWithStdinEmptyInput) {
  auto [exitCode, output] = utils_.runCommandWithStdin({"/bin/cat"}, "");
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "");
}

TEST_F(PlatformUtilsTest, RunCommandWithStdinMultiline) {
  std::string input = "line1\nline2\nline3\n";
  auto [exitCode, output] = utils_.runCommandWithStdin({"/bin/cat"}, input);
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, input);
}

TEST_F(PlatformUtilsTest, RunCommandWithStdinTransform) {
  auto [exitCode, output] =
      utils_.runCommandWithStdin({"/usr/bin/tr", "a-z", "A-Z"}, "hello world");
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "HELLO WORLD");
}

TEST_F(PlatformUtilsTest, RunCommandWithStdinInvalidCommand) {
  auto [exitCode, output] =
      utils_.runCommandWithStdin({"/nonexistent/command"}, "input");
  EXPECT_EQ(exitCode, -1);
  EXPECT_FALSE(output.empty());
}

TEST_F(PlatformUtilsTest, RunCommandWithStdinGrepMatch) {
  std::string input = "apple\nbanana\ncherry\napricot\n";
  auto [exitCode, output] =
      utils_.runCommandWithStdin({"/bin/grep", "ap"}, input);
  EXPECT_EQ(exitCode, 0);
  EXPECT_EQ(output, "apple\napricot\n");
}

TEST_F(PlatformUtilsTest, RunCommandWithStdinGrepNoMatch) {
  std::string input = "apple\nbanana\ncherry\n";
  auto [exitCode, output] =
      utils_.runCommandWithStdin({"/bin/grep", "xyz"}, input);
  EXPECT_NE(exitCode, 0); // grep returns non-zero when no match found
}

} // namespace facebook::fboss::platform
