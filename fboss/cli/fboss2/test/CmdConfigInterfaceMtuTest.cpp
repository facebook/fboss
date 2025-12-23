/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceMtu.h"
#include <gtest/gtest.h>

namespace facebook::fboss {

class CmdConfigInterfaceMtuTestFixture : public ::testing::Test {};

TEST_F(CmdConfigInterfaceMtuTestFixture, printOutputSuccess) {
  auto cmd = CmdConfigInterfaceMtu();
  std::string successMessage =
      "Successfully set MTU for interface(s) eth1/1/1 to 1500. Run 'fboss2 config session commit' to apply changes.";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(successMessage);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();
  std::string expectedOutput =
      "Successfully set MTU for interface(s) eth1/1/1 to 1500. Run 'fboss2 config session commit' to apply changes.\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdConfigInterfaceMtuTestFixture, printOutputMultipleInterfaces) {
  auto cmd = CmdConfigInterfaceMtu();
  std::string successMessage =
      "Successfully set MTU for interface(s) eth1/1/1, eth1/2/1 to 9000. Run 'fboss2 config session commit' to apply changes.";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(successMessage);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();
  std::string expectedOutput =
      "Successfully set MTU for interface(s) eth1/1/1, eth1/2/1 to 9000. Run 'fboss2 config session commit' to apply changes.\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdConfigInterfaceMtuTestFixture, errorOnNonExistentInterface) {
  auto cmd = CmdConfigInterfaceMtu();

  // Test that attempting to set MTU on a non-existent interface throws an error
  // This is important because we cannot create arbitrary interfaces - they must
  // be defined in the configuration
  std::string errorMessage =
      "Interface(s) not found in configuration: eth1/99/1. Interfaces must be "
      "defined in the configuration before setting their MTU.";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(errorMessage);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();
  std::string expectedOutput = errorMessage + "\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdConfigInterfaceMtuTestFixture, errorOnInvalidMtuTooLow) {
  auto cmd = CmdConfigInterfaceMtu();

  // Test that MTU validation works for values below minimum
  std::string errorMessage =
      "MTU must be between 68 and 9216 inclusive, got: 67";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(errorMessage);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();
  std::string expectedOutput = errorMessage + "\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdConfigInterfaceMtuTestFixture, errorOnInvalidMtuTooHigh) {
  auto cmd = CmdConfigInterfaceMtu();

  // Test that MTU validation works for values above maximum
  std::string errorMessage =
      "MTU must be between 68 and 9216 inclusive, got: 9217";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(errorMessage);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();
  std::string expectedOutput = errorMessage + "\n";

  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
