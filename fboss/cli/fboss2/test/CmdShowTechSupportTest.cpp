// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include "fboss/cli/fboss2/commands/show/facebook/techsupport/CmdShowTechSupport.h"
#include "fboss/cli/fboss2/commands/show/facebook/techsupport/gen-cpp2/model_types.h"

using namespace ::testing;

namespace facebook::fboss {

FabricReachabilityStats createReachabilityStats() {
  FabricReachabilityStats stats;
  stats.mismatchCount() = 0;
  stats.missingCount() = 0;
  return stats;
}

CmdShowTechSupport::RetType createTechSupportModel(
    const std::optional<std::string>& agentBootType,
    const std::map<int16_t, std::string>& hwAgentBootType) {
  CmdShowTechSupport cmd;
  ServiceInfo agentInfo;
  agentInfo.version = "agent-revision";

  std::map<int16_t, bool> hwAgentStatus;
  std::map<int16_t, std::string> hwAgentVersion;
  for (const auto& hwAgentBootTypeEntry : hwAgentBootType) {
    hwAgentStatus[hwAgentBootTypeEntry.first] = true;
    hwAgentVersion[hwAgentBootTypeEntry.first] = "hw-agent-version";
  }

  return cmd.createModel(
      {},
      agentInfo,
      {},
      {},
      {},
      {},
      {},
      std::nullopt,
      std::nullopt,
      {},
      {},
      createReachabilityStats(),
      {},
      {},
      {},
      {},
      {},
      {},
      hwAgentStatus,
      hwAgentVersion,
      agentBootType,
      hwAgentBootType);
}

std::string captureTechSupportOutput(CmdShowTechSupport::RetType model) {
  CmdShowTechSupport cmd;
  testing::internal::CaptureStdout();
  cmd.printOutput(model);
  return testing::internal::GetCapturedStdout();
}

std::string getLineContaining(
    const std::string& output,
    const std::string& needle) {
  std::stringstream stream(output);
  std::string line;
  while (std::getline(stream, line)) {
    if (line.find(needle) != std::string::npos) {
      return line;
    }
  }
  return "";
}

TEST(CmdShowTechSupportTest, createModelSetsAgentBootType) {
  auto model = createTechSupportModel("WARM_BOOT", {});

  EXPECT_EQ(model.agentBootType().value(), "WARM_BOOT");
  EXPECT_TRUE(model.hwAgentBootType()->empty());
}

TEST(CmdShowTechSupportTest, printOutputAlignsBootTypesInServiceTable) {
  auto model = createTechSupportModel(
      "WARM_BOOT",
      {
          {0, "COLD_BOOT"},
          {1, "WARM_BOOT"},
      });

  const auto output = captureTechSupportOutput(std::move(model));
  const auto header = getLineContaining(output, "Boot Type");
  const auto agent = getLineContaining(output, "Agent");
  const auto hwAgent0 = getLineContaining(output, "Hw-Agent 0");
  const auto hwAgent1 = getLineContaining(output, "Hw-Agent 1");

  ASSERT_NE(header, "");
  ASSERT_NE(agent, "");
  ASSERT_NE(hwAgent0, "");
  ASSERT_NE(hwAgent1, "");

  const auto bootTypeColumn = header.find("Boot Type");
  EXPECT_EQ(agent.find("WARM_BOOT"), bootTypeColumn);
  EXPECT_EQ(hwAgent0.find("COLD_BOOT"), bootTypeColumn);
  EXPECT_EQ(hwAgent1.find("WARM_BOOT"), bootTypeColumn);
}

} // namespace facebook::fboss
