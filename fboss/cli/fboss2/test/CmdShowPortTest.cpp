// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/commands/show/port/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::map<int32_t, PortInfoThrift> createPortEntries() {
  std::map<int32_t, PortInfoThrift> portMap;

  PortInfoThrift portEntry1;
  portEntry1.portId_ref() = 1;
  portEntry1.name_ref() = "eth1/5/1";
  portEntry1.adminState_ref() = PortAdminState::ENABLED;
  portEntry1.operState_ref() = PortOperState::DOWN;
  portEntry1.speedMbps_ref() = 100000;
  portEntry1.profileID_ref() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  TransceiverIdxThrift tcvr1;
  tcvr1.transceiverId_ref() = 0;
  portEntry1.transceiverIdx_ref() = tcvr1;

  PortInfoThrift portEntry2;
  portEntry2.portId_ref() = 2;
  portEntry2.name_ref() = "eth1/5/2";
  portEntry2.adminState_ref() = PortAdminState::DISABLED;
  portEntry2.operState_ref() = PortOperState::DOWN;
  portEntry2.speedMbps_ref() = 25000;
  portEntry2.profileID_ref() = "PROFILE_25G_1_NRZ_CL74_COPPER";
  TransceiverIdxThrift tcvr2;
  tcvr2.transceiverId_ref() = 1;
  portEntry2.transceiverIdx_ref() = tcvr2;

  PortInfoThrift portEntry3;
  portEntry3.portId_ref() = 3;
  portEntry3.name_ref() = "eth1/5/3";
  portEntry3.adminState_ref() = PortAdminState::ENABLED;
  portEntry3.operState_ref() = PortOperState::UP;
  portEntry3.speedMbps_ref() = 100000;
  portEntry3.profileID_ref() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  TransceiverIdxThrift tcvr3;
  tcvr3.transceiverId_ref() = 2;
  portEntry3.transceiverIdx_ref() = tcvr3;

  portMap[portEntry1.get_portId()] = portEntry1;
  portMap[portEntry2.get_portId()] = portEntry2;
  portMap[portEntry3.get_portId()] = portEntry3;
  return portMap;
}

cli::ShowPortModel createPortModel() {
  cli::ShowPortModel model;

  cli::PortEntry entry1, entry2, entry3;
  entry1.id_ref() = 1;
  entry1.name_ref() = "eth1/5/1";
  entry1.adminState_ref() = "Enabled";
  entry1.operState_ref() = "Down";
  entry1.speed_ref() = "100G";
  entry1.profileId_ref() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  entry1.tcvrID_ref() = 0;

  entry2.id_ref() = 2;
  entry2.name_ref() = "eth1/5/2";
  entry2.adminState_ref() = "Disabled";
  entry2.operState_ref() = "Down";
  entry2.speed_ref() = "25G";
  entry2.profileId_ref() = "PROFILE_25G_1_NRZ_CL74_COPPER";
  entry2.tcvrID_ref() = 1;

  entry3.id_ref() = 3;
  entry3.name_ref() = "eth1/5/3";
  entry3.adminState_ref() = "Enabled";
  entry3.operState_ref() = "Up";
  entry3.speed_ref() = "100G";
  entry3.profileId_ref() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  entry3.tcvrID_ref() = 2;

  model.portEntries_ref() = {entry1, entry2, entry3};
  return model;
}

class CmdShowPortTestFixture : public CmdHandlerTestBase {
 public:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  cli::ShowPortModel normalizedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    portEntries = createPortEntries();
    normalizedModel = createPortModel();
  }
};

TEST_F(CmdShowPortTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));

  auto cmd = CmdShowPort();
  CmdShowPortTraits::ObjectArgType queriedEntries;
  auto model = cmd.queryClient(localhost(), queriedEntries);

  EXPECT_THRIFT_EQ(model, normalizedModel);
}

TEST_F(CmdShowPortTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowPort().printOutput(normalizedModel, ss);

  std::string output = ss.str();
  std::string expectOutput =
      " ID  Name      AdminState  LinkState  TcvrID  Speed  ProfileID                      \n"
      "--------------------------------------------------------------------------------------------\n"
      " 1   eth1/5/1  Enabled     Down       0       100G   PROFILE_100G_4_NRZ_CL91_COPPER \n"
      " 2   eth1/5/2  Disabled    Down       1       25G    PROFILE_25G_1_NRZ_CL74_COPPER  \n"
      " 3   eth1/5/3  Enabled     Up         2       100G   PROFILE_100G_4_NRZ_CL91_COPPER \n\n";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
