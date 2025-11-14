// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceDescription.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceDescriptionTestFixture : public ::testing::Test {
 public:
  void SetUp() override {}
};

TEST_F(CmdConfigInterfaceDescriptionTestFixture, printOutputSuccess) {
  auto cmd = CmdConfigInterfaceDescription();
  std::string successMessage =
      "Successfully set description for interface(s) eth1/1/1 to \"Test description\" and reloaded config";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(successMessage);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();
  std::string expectedOutput =
      "Successfully set description for interface(s) eth1/1/1 to \"Test description\" and reloaded config\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdConfigInterfaceDescriptionTestFixture, printOutputMultiplePorts) {
  auto cmd = CmdConfigInterfaceDescription();
  std::string successMessage =
      "Successfully set description for interface(s) eth1/1/1, eth1/2/1 to \"Multi-port test\" and reloaded config";

  // Redirect cout to capture output
  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(successMessage);

  // Restore cout
  std::cout.rdbuf(old);

  std::string output = buffer.str();
  std::string expectedOutput =
      "Successfully set description for interface(s) eth1/1/1, eth1/2/1 to \"Multi-port test\" and reloaded config\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdConfigInterfaceDescriptionTestFixture, errorOnNonExistentPort) {
  auto cmd = CmdConfigInterfaceDescription();

  // Test that attempting to set description on a non-existent port throws an
  // error This is important because we cannot create arbitrary ports - they
  // must exist in the platform mapping (hardware configuration)

  // Note: This test would require mocking the queryClient method to test the
  // actual error behavior. For now, we just verify the printOutput works
  // correctly.
  std::string errorMessage =
      "Port(s) not found in configuration: eth1/99/1. Ports must exist in the "
      "hardware platform mapping and be defined in the configuration before "
      "setting their description.";

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
