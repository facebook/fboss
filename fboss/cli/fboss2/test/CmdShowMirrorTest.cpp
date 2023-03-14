// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/mirror/CmdShowMirror.h"
#include "fboss/cli/fboss2/commands/show/mirror/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

std::string createMirrorMap() {
  return "{\"fboss_dcflow_traffic_mirror\":{\"name\":\"fboss_dcflow_traffic_mirror\",\"dscp\":42,\"truncate\":true,\"configHasEgressPort\":true,\"egressPort\":1,\"destinationIp\":{\"addr\":\"Cr//Rg\"},\"srcIp\":{\"addr\":\"CqOAvg\"},\"udpSrcPort\":12355,\"udpDstPort\":6346,\"isResolved\":true}}";
}

std::map<int32_t, PortInfoThrift> createPortInfoEntries() {
  std::map<int32_t, PortInfoThrift> portInfoMap;

  PortInfoThrift portInfoEntry1;
  portInfoEntry1.portId() = 1;
  portInfoEntry1.name() = "eth1/5/1";

  PortInfoThrift portInfoEntry2;
  portInfoEntry2.portId() = 5;
  portInfoEntry2.name() = "eth1/2/1";

  PortInfoThrift portInfoEntry3;
  portInfoEntry3.portId() = 10;
  portInfoEntry3.name() = "eth1/6/1";

  portInfoMap[portInfoEntry1.get_portId()] = portInfoEntry1;
  portInfoMap[portInfoEntry2.get_portId()] = portInfoEntry2;
  portInfoMap[portInfoEntry3.get_portId()] = portInfoEntry3;
  return portInfoMap;
}

cli::ShowMirrorModel createExpectedMirrorModel() {
  cli::ShowMirrorModel model;

  cli::ShowMirrorModelEntry modelEntry;
  modelEntry.mirror() = "fboss_dcflow_traffic_mirror";
  modelEntry.status() = "Active";
  modelEntry.egressPort() = "1";
  modelEntry.egressPortName() = "eth1/5/1";
  modelEntry.mirrorTunnelType() = "-";
  modelEntry.srcMAC() = "-";
  modelEntry.srcIP() = "10.163.128.190";
  modelEntry.srcUDPPort() = "12355";
  modelEntry.dstMAC() = "-";
  modelEntry.dstIP() = "10.191.255.70";
  modelEntry.dstUDPPort() = "6346";
  modelEntry.dscp() = "42";

  model.mirrorEntries()->push_back(modelEntry);
  return model;
}

class CmdShowMirrorTestFixture : public CmdHandlerTestBase {
 public:
  std::string mockMirrorMap;
  std::map<int32_t, PortInfoThrift> mockPortInfoEntries;
  cli::ShowMirrorModel expectedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockMirrorMap = createMirrorMap();
    mockPortInfoEntries = createPortInfoEntries();
    expectedModel = createExpectedMirrorModel();
  }
};

TEST_F(CmdShowMirrorTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getCurrentStateJSON(_, _))
      .WillOnce(Invoke([&](std::string& ret, std::unique_ptr<std::string>) {
        ret = mockMirrorMap;
      }));
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = mockPortInfoEntries; }));

  auto cmd = CmdShowMirror();
  auto model = cmd.queryClient(localhost());
  EXPECT_THRIFT_EQ(model, expectedModel);
}

TEST_F(CmdShowMirrorTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowMirror().printOutput(expectedModel, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      " Mirror                       Status  Egress Port  Egress Port Name  Tunnel Type  Src MAC  Src IP          Src UDP Port  Dst MAC  Dst IP         Dst UDP Port  DSCP \n"
      "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
      " fboss_dcflow_traffic_mirror  Active  1            eth1/5/1          -            -        10.163.128.190  12355         -        10.191.255.70  6346          42   \n\n";

  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
