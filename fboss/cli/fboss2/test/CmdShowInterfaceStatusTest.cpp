// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/interface/status/CmdShowInterfaceStatus.h"
#include "fboss/cli/fboss2/commands/show/interface/status/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

std::map<int32_t, facebook::fboss::PortInfoThrift> createStatusPorts() {
  std::map<int32_t, facebook::fboss::PortInfoThrift> portMap;

  facebook::fboss::PortInfoThrift portEntry1;
  portEntry1.portId() = 1;
  portEntry1.name() = "eth1/1/1";
  portEntry1.description() = "u-001: ssw001.s001 (F=spine:L=d-050)";
  portEntry1.operState() = facebook::fboss::PortOperState::UP;
  portEntry1.vlans() = {2001};
  portEntry1.speedMbps() = 100000;

  facebook::fboss::TransceiverIdxThrift transceiverId1;
  transceiverId1.transceiverId() = 100;
  portEntry1.transceiverIdx() = transceiverId1;

  facebook::fboss::PortInfoThrift portEntry2;
  portEntry2.portId() = 2;
  portEntry2.name() = "eth2/1/1";
  portEntry2.description() =
      "d-043: rsw043.p050 (F=rack:L=u-001:D=xyz2.0001.0050.0043)";
  portEntry2.operState() = facebook::fboss::PortOperState::UP;
  portEntry2.vlans() = {2002};
  portEntry2.speedMbps() = 200000;

  facebook::fboss::TransceiverIdxThrift transceiverId2;
  transceiverId2.transceiverId() = 101;
  portEntry2.transceiverIdx() = transceiverId2;

  facebook::fboss::PortInfoThrift portEntry3;
  portEntry3.portId() = 3;
  portEntry3.name() = "eth3/1/1";
  portEntry3.description() = "u-044: unused";
  portEntry3.operState() = facebook::fboss::PortOperState::DOWN;
  portEntry3.vlans() = {2003};
  portEntry3.speedMbps() = 400000;

  facebook::fboss::TransceiverIdxThrift transceiverId3;
  transceiverId3.transceiverId() = 102;
  portEntry3.transceiverIdx() = transceiverId3;

  portMap[portEntry1.get_portId()] = portEntry1;
  portMap[portEntry2.get_portId()] = portEntry2;
  portMap[portEntry3.get_portId()] = portEntry3;

  return portMap;
}

std::map<int32_t, facebook::fboss::TransceiverInfo> createStatusTransceivers() {
  std::map<int32_t, facebook::fboss::TransceiverInfo> transceivers;

  facebook::fboss::TransceiverInfo transceiver1;
  facebook::fboss::Vendor vendor1;
  vendor1.name() = "INTEL CORP";
  vendor1.partNumber() = "SPTSBP2CLCKS";
  transceiver1.vendor() = vendor1;

  facebook::fboss::TransceiverInfo transceiver2;
  facebook::fboss::Vendor vendor2;
  vendor2.name() = "INNOLIGHT";
  vendor2.partNumber() = "TR-FC13H-HFB";
  transceiver2.vendor() = vendor2;

  transceivers[100] = transceiver1;
  transceivers[101] = transceiver2;

  return transceivers;
}

class CmdShowInterfaceStatusTestFixture : public CmdHandlerTestBase {
 public:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  std::map<int32_t, facebook::fboss::TransceiverInfo> transceiverInfo;
  std::vector<std::string> queriedEntries;
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    portEntries = createStatusPorts();
    transceiverInfo = createStatusTransceivers();
  }
};

TEST_F(CmdShowInterfaceStatusTestFixture, createModel) {
  auto cmd = CmdShowInterfaceStatus();
  auto model = cmd.createModel(portEntries, transceiverInfo, queriedEntries);
  auto statusModel = model.get_interfaces();

  EXPECT_EQ(statusModel.size(), 3);

  EXPECT_EQ(statusModel[0].get_name(), "eth1/1/1");
  ASSERT_TRUE(statusModel[0].vlan().has_value());
  EXPECT_EQ(*statusModel[0].get_vlan(), 2001);
  EXPECT_EQ(statusModel[0].get_speed(), "100G");
  EXPECT_EQ(statusModel[0].get_vendor(), "INTEL CORP");
  EXPECT_EQ(statusModel[0].get_mpn(), "SPTSBP2CLCKS");

  EXPECT_EQ(statusModel[1].get_name(), "eth2/1/1");
  ASSERT_TRUE(statusModel[1].vlan().has_value());
  EXPECT_EQ(*statusModel[1].get_vlan(), 2002);
  EXPECT_EQ(statusModel[1].get_speed(), "200G");
  EXPECT_EQ(statusModel[1].get_vendor(), "INNOLIGHT");
  EXPECT_EQ(statusModel[1].get_mpn(), "TR-FC13H-HFB");

  EXPECT_EQ(statusModel[2].get_name(), "eth3/1/1");
  ASSERT_TRUE(statusModel[2].vlan().has_value());
  EXPECT_EQ(*statusModel[2].get_vlan(), 2003);
  EXPECT_EQ(statusModel[2].get_speed(), "400G");
  EXPECT_EQ(statusModel[2].get_vendor(), "Not Present");
  EXPECT_EQ(statusModel[2].get_mpn(), "Not Present");
}

TEST_F(CmdShowInterfaceStatusTestFixture, printOutput) {
  auto cmd = CmdShowInterfaceStatus();
  auto model = cmd.createModel(portEntries, transceiverInfo, queriedEntries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();

  std::string expectedOutput =
      " Interface  Description                                                Status  Vlan  Speed  Vendor       Part Number  \n"
      "------------------------------------------------------------------------------------------------------------------------------\n"
      " eth1/1/1   u-001: ssw001.s001 (F=spine:L=d-050)                       up      2001  100G   INTEL CORP   SPTSBP2CLCKS \n"
      " eth2/1/1   d-043: rsw043.p050 (F=rack:L=u-001:D=xyz2.0001.0050.0043)  up      2002  200G   INNOLIGHT    TR-FC13H-HFB \n"
      " eth3/1/1   u-044: unused                                              down    2003  400G   Not Present  Not Present  \n\n";

  EXPECT_EQ(output, expectedOutput);
}
} // namespace facebook::fboss
