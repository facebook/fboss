/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/json/json.h>

#include "fboss/cli/fboss2/commands/show/bgp/config/CmdShowConfigRunningBgp.h"

using namespace ::testing;

// Covers the OSS implementation of `show config running bgp`, which fetches the
// daemon's raw running-config JSON via getRunningConfig() and pretty-prints it.
// This is OSS/CMake-only: the internal (Buck) build links the structured
// getRunningConfigStruct() implementation instead, which is covered separately
// by test/facebook/CmdShowConfigRunningTest.cpp.
namespace facebook::fboss {
class CmdShowConfigRunningBgpTestFixture : public CmdHandlerTestBase {
 public:
  std::string mockBgpRunningConfig;
  folly::dynamic expectedConfigRunningBgpOutput;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockBgpRunningConfig = getBgpRunningConfig();
    expectedConfigRunningBgpOutput = folly::parseJson(mockBgpRunningConfig);
  }

 private:
  std::string getBgpRunningConfig() {
    folly::dynamic bgpRunningConfig = folly::dynamic::object(
        "router_id", "10.170.224.128")("local_as", 65401);
    return folly::toJson(bgpRunningConfig);
  }
};

TEST_F(CmdShowConfigRunningBgpTestFixture, queryBgpClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRunningConfig(_))
      .WillOnce(Invoke(
          [&](std::string& configStr) { configStr = mockBgpRunningConfig; }));

  auto cmd = CmdShowConfigRunningBgp();
  auto configRunningBgpOutput = cmd.queryClient(localhost());
  EXPECT_EQ(configRunningBgpOutput, expectedConfigRunningBgpOutput);
}

TEST_F(CmdShowConfigRunningBgpTestFixture, printBgpOutput) {
  std::stringstream ss;
  CmdShowConfigRunningBgp().printOutput(expectedConfigRunningBgpOutput, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      folly::toPrettyJson(expectedConfigRunningBgpOutput);
  expectedOutput.append("\n");

  EXPECT_EQ(output, expectedOutput);
}
} // namespace facebook::fboss
