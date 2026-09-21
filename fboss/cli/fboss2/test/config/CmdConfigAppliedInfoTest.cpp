// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

#include "fboss/cli/fboss2/commands/config/CmdConfigAppliedInfo.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

ConfigAppliedInfo createMockConfigAppliedInfoWarmboot() {
  ConfigAppliedInfo configAppliedInfo;
  configAppliedInfo.lastAppliedInMs() = 1609459200000;
  return configAppliedInfo;
}

ConfigAppliedInfo createMockConfigAppliedInfo() {
  auto configAppliedInfo = createMockConfigAppliedInfoWarmboot();
  configAppliedInfo.lastColdbootAppliedInMs() = 1609459100000;
  return configAppliedInfo;
}

class CmdConfigAppliedInfoTestFixture : public CmdHandlerTestBase {
 public:
  ConfigAppliedInfo mockConfigAppliedInfo;
  ConfigAppliedInfo mockConfigAppliedInfoWarmboot;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockConfigAppliedInfo = createMockConfigAppliedInfo();
    mockConfigAppliedInfoWarmboot = createMockConfigAppliedInfoWarmboot();
  }
};

TEST_F(CmdConfigAppliedInfoTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getConfigAppliedInfo(_))
      .WillOnce(Invoke([&](auto& configAppliedInfo) {
        configAppliedInfo = mockConfigAppliedInfo;
      }));

  auto cmd = CmdConfigAppliedInfo();
  auto result = cmd.queryClient(localhost());

  EXPECT_EQ(
      *result.lastAppliedInMs(), *mockConfigAppliedInfo.lastAppliedInMs());
  EXPECT_EQ(
      *result.lastColdbootAppliedInMs(),
      *mockConfigAppliedInfo.lastColdbootAppliedInMs());
}

TEST_F(CmdConfigAppliedInfoTestFixture, queryClientWarmboot) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getConfigAppliedInfo(_))
      .WillOnce(Invoke([&](auto& configAppliedInfo) {
        configAppliedInfo = mockConfigAppliedInfoWarmboot;
      }));

  auto cmd = CmdConfigAppliedInfo();
  auto result = cmd.queryClient(localhost());

  EXPECT_EQ(
      *result.lastAppliedInMs(),
      *mockConfigAppliedInfoWarmboot.lastAppliedInMs());
  EXPECT_FALSE(result.lastColdbootAppliedInMs().has_value());
}

TEST_F(CmdConfigAppliedInfoTestFixture, printOutputWithColdboot) {
  auto cmd = CmdConfigAppliedInfo();

  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(mockConfigAppliedInfo);

  std::cout.rdbuf(old);

  std::string output = buffer.str();

  EXPECT_NE(output.find("Config Applied Information:"), std::string::npos);
  EXPECT_NE(output.find("==========================="), std::string::npos);
  EXPECT_NE(output.find("Last Applied Time:"), std::string::npos);
  EXPECT_NE(output.find("Last Coldboot Applied Time:"), std::string::npos);
  EXPECT_EQ(output.find("Not available (warmboot)"), std::string::npos);
}

TEST_F(CmdConfigAppliedInfoTestFixture, printOutputWarmboot) {
  auto cmd = CmdConfigAppliedInfo();

  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(mockConfigAppliedInfoWarmboot);

  std::cout.rdbuf(old);

  std::string output = buffer.str();

  EXPECT_NE(output.find("Config Applied Information:"), std::string::npos);
  EXPECT_NE(output.find("==========================="), std::string::npos);
  EXPECT_NE(output.find("Last Applied Time:"), std::string::npos);
  EXPECT_NE(output.find("Last Coldboot Applied Time:"), std::string::npos);
  EXPECT_NE(output.find("Not available (warmboot)"), std::string::npos);
}

TEST_F(CmdConfigAppliedInfoTestFixture, printOutputZeroTimestamp) {
  auto cmd = CmdConfigAppliedInfo();

  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

  cmd.printOutput(ConfigAppliedInfo()); // Zero value.

  std::cout.rdbuf(old);

  std::string output = buffer.str();

  EXPECT_NE(output.find("Config Applied Information:"), std::string::npos);
  EXPECT_NE(output.find("==========================="), std::string::npos);
  EXPECT_NE(output.find("Last Applied Time:"), std::string::npos);
  EXPECT_NE(output.find("Never"), std::string::npos);
}

} // namespace facebook::fboss
