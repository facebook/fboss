// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/mirror/CmdShowMirror.h"
#include "fboss/cli/fboss2/commands/show/mirror/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

std::string createMirrorMapsWithoutTunnel() {
  return "{\"id=0\":{\"mirror_without_tunnel\":{\"name\":\"mirror_without_tunnel\",\"dscp\":42,\"truncate\":true,\"configHasEgressPort\":true,\"egressPortDesc\":{\"portId\":1,\"portType\":0},\"destinationIp\":{\"addr\":\"Cr//Rg\"},\"srcIp\":{\"addr\":\"CqOAvg\"},\"udpSrcPort\":12355,\"udpDstPort\":6346,\"isResolved\":true}}}";
}

std::string createMirrorMapsWithTunnel() {
  return "{\"id=0\":{\"mirror_with_tunnel\":{\"name\":\"mirror_with_tunnel\",\"dscp\":10,\"truncate\":false,\"configHasEgressPort\":false,\"egressPortDesc\":{\"portId\":5,\"portType\":0},\"destinationIp\":{\"addr\":\"AQIDBA\"},\"tunnel\":{\"srcIp\":{\"addr\":\"Co2RIQ\"},\"dstIp\":{\"addr\":\"AQIDBA\"},\"srcMac\":\"b6:a9:fc:34:2d:a2\",\"dstMac\":\"b6:a9:fc:34:31:20\",\"ttl\":255},\"isResolved\":true}}}";
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

  PortInfoThrift portInfoEntry4;
  portInfoEntry4.portId() = 40000;
  portInfoEntry4.name() = "eth1/7/1";

  portInfoMap[folly::copy(portInfoEntry1.portId().value())] = portInfoEntry1;
  portInfoMap[folly::copy(portInfoEntry2.portId().value())] = portInfoEntry2;
  portInfoMap[folly::copy(portInfoEntry3.portId().value())] = portInfoEntry3;
  portInfoMap[folly::copy(portInfoEntry4.portId().value())] = portInfoEntry4;
  return portInfoMap;
}

cli::ShowMirrorModel createExpectedMirrorWithoutTunnelModel() {
  cli::ShowMirrorModel model;

  cli::ShowMirrorModelEntry modelEntry;
  modelEntry.mirror() = "mirror_without_tunnel";
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
  modelEntry.ttl() = "-";

  model.mirrorEntries()->push_back(modelEntry);
  return model;
}

cli::ShowMirrorModel createExpectedMirrorWithTunnelModel() {
  cli::ShowMirrorModel model;

  cli::ShowMirrorModelEntry modelEntry;
  modelEntry.mirror() = "mirror_with_tunnel";
  modelEntry.status() = "Active";
  modelEntry.egressPort() = "5";
  modelEntry.egressPortName() = "eth1/2/1";
  modelEntry.mirrorTunnelType() = "GRE";
  modelEntry.srcMAC() = "b6:a9:fc:34:2d:a2";
  modelEntry.srcIP() = "10.141.145.33";
  modelEntry.srcUDPPort() = "-";
  modelEntry.dstMAC() = "b6:a9:fc:34:31:20";
  modelEntry.dstIP() = "1.2.3.4";
  modelEntry.dstUDPPort() = "-";
  modelEntry.dscp() = "10";
  modelEntry.ttl() = "255";

  model.mirrorEntries()->push_back(modelEntry);
  return model;
}

cli::ShowMirrorModel createExpectedMirrorWithHighPort() {
  cli::ShowMirrorModel model;

  cli::ShowMirrorModelEntry modelEntry;
  modelEntry.mirror() = "mirror_with_tunnel";
  modelEntry.status() = "Active";
  modelEntry.egressPort() = "40000";
  modelEntry.egressPortName() = "eth1/7/1";
  modelEntry.mirrorTunnelType() = "GRE";
  modelEntry.srcMAC() = "b6:a9:fc:34:2d:a2";
  modelEntry.srcIP() = "10.141.145.33";
  modelEntry.srcUDPPort() = "-";
  modelEntry.dstMAC() = "b6:a9:fc:34:31:20";
  modelEntry.dstIP() = "1.2.3.4";
  modelEntry.dstUDPPort() = "-";
  modelEntry.dscp() = "10";
  modelEntry.ttl() = "255";

  model.mirrorEntries()->push_back(modelEntry);
  return model;
}

class CmdShowMirrorTestFixture : public CmdHandlerTestBase {
 public:
  CmdShowMirrorTraits::ObjectArgType queriedMirrors;
  std::string mockMirrorMapsWithoutTunnel;
  std::string mockMirrorMapsWithTunnel;
  std::map<int32_t, PortInfoThrift> mockPortInfoEntries;
  cli::ShowMirrorModel expectedWithoutTunnelModel;
  cli::ShowMirrorModel expectedWithTunnelModel;
  cli::ShowMirrorModel expectedWithHighPort;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockMirrorMapsWithoutTunnel = createMirrorMapsWithoutTunnel();
    mockMirrorMapsWithTunnel = createMirrorMapsWithTunnel();
    mockPortInfoEntries = createPortInfoEntries();
    expectedWithoutTunnelModel = createExpectedMirrorWithoutTunnelModel();
    expectedWithTunnelModel = createExpectedMirrorWithTunnelModel();
    expectedWithHighPort = createExpectedMirrorWithHighPort();
  }
};

TEST_F(CmdShowMirrorTestFixture, queryClientWithoutTunnel) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getCurrentStateJSON(_, _))
      .WillOnce(Invoke([&](std::string& ret, std::unique_ptr<std::string>) {
        ret = mockMirrorMapsWithoutTunnel;
      }));
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = mockPortInfoEntries; }));

  auto cmd = CmdShowMirror();
  auto model = cmd.queryClient(localhost(), queriedMirrors);
  EXPECT_THRIFT_EQ(expectedWithoutTunnelModel, model);
}

TEST_F(CmdShowMirrorTestFixture, printOutputWithoutTunnel) {
  std::stringstream ss;
  CmdShowMirror().printOutput(expectedWithoutTunnelModel, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      " Mirror                 Status  Egress Port  Egress Port Name  Tunnel Type  Src MAC  Src IP          Src UDP Port  Dst MAC  Dst IP         Dst UDP Port  DSCP  TTL \n"
      "---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
      " mirror_without_tunnel  Active  1            eth1/5/1          -            -        10.163.128.190  12355         -        10.191.255.70  6346          42    -   \n\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowMirrorTestFixture, queryClientWithTunnel) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getCurrentStateJSON(_, _))
      .WillOnce(Invoke([&](std::string& ret, std::unique_ptr<std::string>) {
        ret = mockMirrorMapsWithTunnel;
      }));
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = mockPortInfoEntries; }));

  auto cmd = CmdShowMirror();
  auto model = cmd.queryClient(localhost(), queriedMirrors);
  EXPECT_THRIFT_EQ(expectedWithTunnelModel, model);
}

TEST_F(CmdShowMirrorTestFixture, printOutputWithTunnel) {
  std::stringstream ss;
  CmdShowMirror().printOutput(expectedWithTunnelModel, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      " Mirror              Status  Egress Port  Egress Port Name  Tunnel Type  Src MAC            Src IP         Src UDP Port  Dst MAC            Dst IP   Dst UDP Port  DSCP  TTL \n"
      "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
      " mirror_with_tunnel  Active  5            eth1/2/1          GRE          b6:a9:fc:34:2d:a2  10.141.145.33  -             b6:a9:fc:34:31:20  1.2.3.4  -             10    255 \n\n";

  EXPECT_EQ(output, expectedOutput);
}

TEST_F(CmdShowMirrorTestFixture, printOutputWithHighPort) {
  std::stringstream ss;
  CmdShowMirror().printOutput(expectedWithHighPort, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      " Mirror              Status  Egress Port  Egress Port Name  Tunnel Type  Src MAC            Src IP         Src UDP Port  Dst MAC            Dst IP   Dst UDP Port  DSCP  TTL \n"
      "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
      " mirror_with_tunnel  Active  40000        eth1/7/1          GRE          b6:a9:fc:34:2d:a2  10.141.145.33  -             b6:a9:fc:34:31:20  1.2.3.4  -             10    255 \n\n";

  EXPECT_EQ(output, expectedOutput);
}
} // namespace facebook::fboss
