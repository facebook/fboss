// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/hwagent/CmdShowHwAgentStatus.h"
#include "fboss/cli/fboss2/commands/show/hwagent/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

HwAgentEventSyncStatus createHwAgentStatusEntry() {
  HwAgentEventSyncStatus statusEntry;
  statusEntry.linkEventSyncActive() = 1;
  statusEntry.fdbEventSyncActive() = 1;
  statusEntry.statsEventSyncActive() = 1;
  statusEntry.statsEventSyncActive() = 1;
  statusEntry.rxPktEventSyncActive() = 1;
  statusEntry.txPktEventSyncActive() = 1;
  return statusEntry;
}

cli::ShowHwAgentStatusModel createHwAgentStatusModel() {
  cli::ShowHwAgentStatusModel model;
  cli::HwAgentStatusEntry statusEntry;
  statusEntry.switchId() = 100;
  statusEntry.switchIndex() = 0;
  statusEntry.runState() = "CONFIGURED";
  statusEntry.linkSyncActive() = 1;
  statusEntry.linkSyncActive() = 1;
  statusEntry.linkSyncActive() = 1;
  statusEntry.statsSyncActive() = 1;
  statusEntry.fdbSyncActive() = 1;
  statusEntry.rxPktSyncActive() = 1;
  statusEntry.txPktSyncActive() = 1;
  model.hwAgentStatusEntries() = {statusEntry};
  return model;
}

class CmdShowHwAgentStatusTestFixture : public CmdHandlerTestBase {
 public:
  CmdShowHwAgentStatusTraits::ObjectArgType queriedEntries;
  std::map<int16_t, HwAgentEventSyncStatus> mockHwAgentStatusEntries;
  cli::ShowHwAgentStatusModel normalizedModel;
  MultiSwitchRunState mockMultiSwitchRunState;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockHwAgentStatusEntries.insert({0, createHwAgentStatusEntry()});
    mockMultiSwitchRunState.hwIndexToRunState()->insert(
        {100, SwitchRunState::CONFIGURED});
    normalizedModel = createHwAgentStatusModel();
  }
};

TEST_F(CmdShowHwAgentStatusTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getHwAgentConnectionStatus(_))
      .WillOnce(
          Invoke([&](auto& entries) { entries = mockHwAgentStatusEntries; }));
  EXPECT_CALL(getMockAgent(), getMultiSwitchRunState(_))
      .WillOnce(
          Invoke([&](auto& entries) { entries = mockMultiSwitchRunState; }));

  auto cmd = CmdShowHwAgentStatus();
  auto model = cmd.queryClient(localhost());

  EXPECT_THRIFT_EQ(model, normalizedModel);
}

} // namespace facebook::fboss
