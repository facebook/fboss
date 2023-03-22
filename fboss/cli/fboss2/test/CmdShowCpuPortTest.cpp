// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/cpuport/CmdShowCpuPort.h"
#include "fboss/cli/fboss2/commands/show/cpuport/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

CpuPortStats createCpuQueueEntries() {
  std::map<int, uint64_t> queueInPackets;
  std::map<int, uint64_t> queueDiscardPackets;
  std::map<int, std::string> queueToName;

  CpuPortStats cpuStats;

  cpuStats.queueToName_()->insert(
      std::pair<int, std::string>(10, "cpuQueue-low"));
  cpuStats.queueToName_()->insert(
      std::pair<int, std::string>(15, "cpuQueue-high"));

  cpuStats.queueInPackets_()->insert(std::pair<int, int64_t>(10, 2000));
  cpuStats.queueInPackets_()->insert(std::pair<int, int64_t>(15, 500));

  cpuStats.queueDiscardPackets_()->insert(std::pair<int, int64_t>(10, 1000));
  cpuStats.queueDiscardPackets_()->insert(std::pair<int, int64_t>(15, 0));

  return cpuStats;
}

cli::ShowCpuPortModel createCpuPortModel() {
  cli::ShowCpuPortModel model;
  cli::CpuPortQueueEntry entry1, entry2;

  entry1.id() = 10;
  entry1.name() = "cpuQueue-low";
  entry1.ingressPackets() = 2000;
  entry1.discardPackets() = 1000;

  entry2.id() = 15;
  entry2.name() = "cpuQueue-high";
  entry2.ingressPackets() = 500;
  entry2.discardPackets() = 0;

  model.cpuQueueEntries() = {entry1, entry2};
  return model;
}

class CmdShowCpuPortTestFixture : public CmdHandlerTestBase {
 public:
  CmdShowCpuPortTraits::ObjectArgType queriedEntries;
  CpuPortStats mockCpuPortEntries;
  cli::ShowCpuPortModel normalizedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockCpuPortEntries = createCpuQueueEntries();
    normalizedModel = createCpuPortModel();
  }
};

TEST_F(CmdShowCpuPortTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getCpuPortStats(_))
      .WillOnce(Invoke([&](auto& entries) { entries = mockCpuPortEntries; }));

  auto cmd = CmdShowCpuPort();
  CmdShowCpuPortTraits::ObjectArgType queriedEntries;
  auto model = cmd.queryClient(localhost());

  EXPECT_THRIFT_EQ(model, normalizedModel);
}

} // namespace facebook::fboss
