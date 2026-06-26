// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/commands/show/systemport/CmdShowSystemPort.h"
#include "fboss/cli/fboss2/commands/show/systemport/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

namespace {

constexpr auto kSystemPortName = "hwTestSwitch0:eth1/63/1";

std::map<int64_t, SystemPortThrift> createSystemPortEntries() {
  SystemPortThrift systemPort;
  systemPort.portId() = 394;
  systemPort.switchId() = 4;
  systemPort.portName() = kSystemPortName;
  systemPort.coreIndex() = 1;
  systemPort.corePortIndex() = 63;
  systemPort.speedMbps() = 400000;
  systemPort.numVoqs() = 8;
  systemPort.scope() = cfg::Scope::GLOBAL;

  return {{*systemPort.portId(), systemPort}};
}

std::map<int64_t, cfg::SwitchInfo> createSwitchInfo() {
  cfg::SwitchInfo switchInfo;
  switchInfo.switchType() = cfg::SwitchType::FABRIC;
  switchInfo.asicType() = cfg::AsicType::ASIC_TYPE_TRIDENT2;
  switchInfo.switchIndex() = 1;

  return {{4, switchInfo}};
}

HwSysPortStats createSysPortStats(int64_t queueOutBytes) {
  HwSysPortStats stats;
  stats.queueOutDiscardBytes_() = {{0, 11}};
  stats.queueOutBytes_() = {{0, queueOutBytes}};
  stats.queueWatermarkBytes_() = {{0, 33}};
  return stats;
}

int64_t getQueueOutBytes(const cli::ShowSystemPortModel& model) {
  return model.sysPortEntries()->front().hwPortStats()->egressOutBytes()->at(0);
}

} // namespace

class CmdShowSystemPortTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowSystemPortTestFixture, createModelUsesSwitchScopedStats) {
  std::map<std::string, HwSysPortStats> sysPortStats{
      {"switch.1.hwTestSwitch0:eth1/63/1", createSysPortStats(22)},
      {kSystemPortName, createSysPortStats(44)},
  };

  auto model = CmdShowSystemPort().createModel(
      createSystemPortEntries(), {}, sysPortStats, createSwitchInfo());

  EXPECT_EQ(*model.sysPortEntries()->front().switchIndex(), 1);
  EXPECT_EQ(getQueueOutBytes(model), 22);
}

TEST_F(CmdShowSystemPortTestFixture, createModelFallsBackToUnscopedStats) {
  std::map<std::string, HwSysPortStats> sysPortStats{
      {kSystemPortName, createSysPortStats(44)},
  };

  auto model = CmdShowSystemPort().createModel(
      createSystemPortEntries(), {}, sysPortStats, createSwitchInfo());

  EXPECT_EQ(*model.sysPortEntries()->front().switchIndex(), 1);
  EXPECT_EQ(getQueueOutBytes(model), 44);
}

} // namespace facebook::fboss
