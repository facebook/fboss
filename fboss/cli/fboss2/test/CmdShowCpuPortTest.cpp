// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include "fboss/cli/fboss2/commands/show/cpuport/CmdShowCpuPort.h"
#include "fboss/cli/fboss2/commands/show/cpuport/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

std::map<int32_t, CpuPortStats> createCpuQueueEntries() {
  std::map<int, uint64_t> queueInPackets;
  std::map<int, uint64_t> queueDiscardPackets;
  std::map<int, std::string> queueToName;

  CpuPortStats sw1CpuStats, sw2CpuStats;
  std::map<int32_t, CpuPortStats> cpuPortStatEntries;

  sw1CpuStats.queueToName_()->insert(
      std::pair<int, std::string>(10, "cpuQueue-low"));
  sw1CpuStats.queueToName_()->insert(
      std::pair<int, std::string>(15, "cpuQueue-high"));
  sw1CpuStats.queueInPackets_()->insert(std::pair<int, int64_t>(10, 2000));
  sw1CpuStats.queueInPackets_()->insert(std::pair<int, int64_t>(15, 500));
  sw1CpuStats.queueDiscardPackets_()->insert(std::pair<int, int64_t>(10, 1000));
  sw1CpuStats.queueDiscardPackets_()->insert(std::pair<int, int64_t>(15, 0));

  sw2CpuStats.queueToName_()->insert(
      std::pair<int, std::string>(100, "cpuQueue-low"));
  sw2CpuStats.queueToName_()->insert(
      std::pair<int, std::string>(115, "cpuQueue-high"));
  sw2CpuStats.queueInPackets_()->insert(std::pair<int, int64_t>(100, 2000));
  sw2CpuStats.queueInPackets_()->insert(std::pair<int, int64_t>(115, 500));
  sw2CpuStats.queueDiscardPackets_()->insert(
      std::pair<int, int64_t>(100, 1000));
  sw2CpuStats.queueDiscardPackets_()->insert(std::pair<int, int64_t>(115, 0));

  cpuPortStatEntries.insert(std::pair<int32_t, CpuPortStats>(1, sw1CpuStats));
  cpuPortStatEntries.insert(std::pair<int32_t, CpuPortStats>(2, sw2CpuStats));
  return cpuPortStatEntries;
}

cli::ShowCpuPortModel createCpuPortModel() {
  cli::ShowCpuPortModel model;
  cli::CpuPortQueueEntry sw1Entry1, sw1Entry2, sw2Entry1, sw2Entry2;

  sw1Entry1.id() = 10;
  sw1Entry1.name() = "cpuQueue-low";
  sw1Entry1.ingressPackets() = 2000;
  sw1Entry1.discardPackets() = 1000;

  sw1Entry2.id() = 15;
  sw1Entry2.name() = "cpuQueue-high";
  sw1Entry2.ingressPackets() = 500;
  sw1Entry2.discardPackets() = 0;

  sw2Entry1.id() = 100;
  sw2Entry1.name() = "cpuQueue-low";
  sw2Entry1.ingressPackets() = 2000;
  sw2Entry1.discardPackets() = 1000;

  sw2Entry2.id() = 115;
  sw2Entry2.name() = "cpuQueue-high";
  sw2Entry2.ingressPackets() = 500;
  sw2Entry2.discardPackets() = 0;

  model.cpuPortStatEntries()[1] = {sw1Entry1, sw1Entry2};
  model.cpuPortStatEntries()[2] = {sw2Entry1, sw2Entry2};
  return model;
}

class CmdShowCpuPortTestFixture : public CmdHandlerTestBase {
 public:
  CmdShowCpuPortTraits::ObjectArgType queriedEntries;
  std::map<int32_t, CpuPortStats> mockCpuPortEntries;
  cli::ShowCpuPortModel normalizedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockCpuPortEntries = createCpuQueueEntries();
    normalizedModel = createCpuPortModel();
  }
};

TEST_F(CmdShowCpuPortTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getAllCpuPortStats(_))
      .WillOnce(Invoke([&](auto& entries) { entries = mockCpuPortEntries; }));

  auto cmd = CmdShowCpuPort();
  auto model = cmd.queryClient(localhost());

  EXPECT_THRIFT_EQ(normalizedModel, model);
}

} // namespace facebook::fboss
