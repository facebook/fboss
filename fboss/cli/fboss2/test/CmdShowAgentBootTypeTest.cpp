// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/commands/show/agent/CmdShowAgentBootType.h"
#include "fboss/cli/fboss2/commands/show/agent/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include <thrift/lib/cpp2/reflection/testing.h>

using namespace ::testing;

namespace facebook::fboss {

MultiSwitchRunState createMultiSwitchRunState(bool multiSwitchEnabled) {
  MultiSwitchRunState runState;
  runState.multiSwitchEnabled() = multiSwitchEnabled;
  if (multiSwitchEnabled) {
    runState.hwIndexToRunState()->insert({100, SwitchRunState::CONFIGURED});
  }
  return runState;
}

cli::ShowAgentBootTypeModel createBootTypeModel(
    std::vector<cli::AgentBootTypeEntry> entries) {
  cli::ShowAgentBootTypeModel model;
  model.bootTypeEntries() = std::move(entries);
  return model;
}

cli::AgentBootTypeEntry createBootTypeEntry(
    const std::string& agentName,
    std::optional<int32_t> switchIndex,
    const std::string& bootType) {
  cli::AgentBootTypeEntry entry;
  entry.agentName() = agentName;
  if (switchIndex.has_value()) {
    entry.switchIndex() = switchIndex.value();
  }
  entry.bootType() = bootType;
  return entry;
}

class CmdShowAgentBootTypeTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowAgentBootTypeTestFixture, queryClientMonoMode) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getMultiSwitchRunState(_))
      .WillOnce(Invoke(
          [](auto& runState) { runState = createMultiSwitchRunState(false); }));
  EXPECT_CALL(getMockAgent(), getBootType())
      .WillOnce(Return(BootType::COLD_BOOT));

  auto cmd = CmdShowAgentBootType();
  const auto model = cmd.queryClient(localhost());
  const auto expectedModel = createBootTypeModel({
      createBootTypeEntry("wedge_agent", std::nullopt, "COLD_BOOT"),
  });

  EXPECT_THRIFT_EQ(expectedModel, model);
}

TEST_F(CmdShowAgentBootTypeTestFixture, queryClientMultiMode) {
  setupMockedAgentServer();
  setupMockedHwAgentServer();
  EXPECT_CALL(getMockAgent(), getMultiSwitchRunState(_))
      .WillOnce(Invoke(
          [](auto& runState) { runState = createMultiSwitchRunState(true); }));
  EXPECT_CALL(getMockAgent(), getBootType())
      .WillOnce(Return(BootType::WARM_BOOT));
  EXPECT_CALL(getMockHwAgent(), getBootType())
      .WillOnce(Return(BootType::COLD_BOOT));

  auto cmd = CmdShowAgentBootType();
  const auto model = cmd.queryClient(localhost());
  const auto expectedModel = createBootTypeModel({
      createBootTypeEntry("sw_agent", std::nullopt, "WARM_BOOT"),
      createBootTypeEntry("hw_agent", 0, "COLD_BOOT"),
  });

  EXPECT_THRIFT_EQ(expectedModel, model);
}

} // namespace facebook::fboss
