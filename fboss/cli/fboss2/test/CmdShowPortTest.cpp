// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/commands/show/port/gen-cpp2/model_types.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::map<int32_t, facebook::fboss::PortInfoThrift> createPortEntries() {
  std::map<int32_t, facebook::fboss::PortInfoThrift> portMap;

  facebook::fboss::PortInfoThrift portEntry1;
  portEntry1.portId_ref() = 1;
  portEntry1.name_ref() = "eth1/5/1";
  portEntry1.adminState_ref() = fboss::PortAdminState::ENABLED;
  portEntry1.operState_ref() = fboss::PortOperState::DOWN;
  portEntry1.speedMbps_ref() = 100000;
  portEntry1.profileID_ref() = "PROFILE_100G_4_NRZ_CL91_COPPER";

  fboss::PortInfoThrift portEntry2;
  portEntry2.portId_ref() = 2;
  portEntry2.name_ref() = "eth1/5/2";
  portEntry2.adminState_ref() = fboss::PortAdminState::DISABLED;
  portEntry2.operState_ref() = fboss::PortOperState::DOWN;
  portEntry2.speedMbps_ref() = 25000;
  portEntry2.profileID_ref() = "PROFILE_25G_1_NRZ_CL74_COPPER";

  fboss::PortInfoThrift portEntry3;
  portEntry3.portId_ref() = 3;
  portEntry3.name_ref() = "eth1/5/3";
  portEntry3.adminState_ref() = fboss::PortAdminState::ENABLED;
  portEntry3.operState_ref() = fboss::PortOperState::UP;
  portEntry3.speedMbps_ref() = 100000;
  portEntry3.profileID_ref() = "PROFILE_100G_4_NRZ_CL91_COPPER";


  portMap[portEntry1.get_portId()] = portEntry1;
  portMap[portEntry2.get_portId()] = portEntry2;
  portMap[portEntry3.get_portId()] = portEntry3;
  return portMap;
}

class CmdShowPortTestFixture : public testing::Test {
 public:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  folly::IPAddressV4 hostIp;

  void SetUp() override {
    portEntries = createPortEntries();
    hostIp = folly::IPAddressV4::tryFromString("127.0.0.1").value();
  }
};

TEST_F(CmdShowPortTestFixture, createModel) {
  auto cmd = CmdShowPort();
  CmdShowPortTraits::ObjectArgType queriedEntries;
  auto model = cmd.createModel(portEntries, queriedEntries);
  auto entries = model.get_portEntries();

  EXPECT_EQ(entries.size(), 3);

  EXPECT_EQ(entries[0].get_id(), 1);
  EXPECT_EQ(entries[0].get_name(), "eth1/5/1");
  EXPECT_EQ(entries[0].get_adminState(), "Enabled");
  EXPECT_EQ(entries[0].get_operState(), "Down");
  EXPECT_EQ(entries[0].get_speed(), "100G");
  EXPECT_EQ(entries[0].get_profileId(), "PROFILE_100G_4_NRZ_CL91_COPPER");

  EXPECT_EQ(entries[1].get_id(), 2);
  EXPECT_EQ(entries[1].get_name(), "eth1/5/2");
  EXPECT_EQ(entries[1].get_adminState(), "Disabled");
  EXPECT_EQ(entries[1].get_operState(), "Down");
  EXPECT_EQ(entries[1].get_speed(), "25G");
  EXPECT_EQ(entries[1].get_profileId(), "PROFILE_25G_1_NRZ_CL74_COPPER");

  EXPECT_EQ(entries[2].get_id(), 3);
  EXPECT_EQ(entries[2].get_name(), "eth1/5/3");
  EXPECT_EQ(entries[2].get_adminState(), "Enabled");
  EXPECT_EQ(entries[2].get_operState(), "Up");
  EXPECT_EQ(entries[2].get_speed(), "100G");
  EXPECT_EQ(entries[2].get_profileId(), "PROFILE_100G_4_NRZ_CL91_COPPER");
}

/*
// TODO. CmdShowPort.printOutput uses fmt::print for color format and output,
// which does not work with ostream. Enable the test after investigating if
// fmt::foramt can provide coloring output.
TEST_F(CmdShowPortTestFixture, printOutput) {
  auto cmd = CmdShowPort();
  CmdShowPortTraits::ObjectArgType queriedEntries;
  auto model = cmd.createModel(portEntries, queriedEntries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
"ID     Name           AdminState     LinkState      Speed     ProfileID           \n"
"------------------------------------------------------------------------------------------\n"
"1      eth1/5/1       Enabled        Down           100G      PROFILE_100G_4_NRZ_CL91_COPPER\n"
"2      eth1/5/2       Disabled       Down           25G       PROFILE_25G_1_NRZ_CL74_COPPER\n"
"3      eth1/5/3       Enabled        Up             100G      PROFILE_100G_4_NRZ_CL91_COPPER\n\n";
  EXPECT_EQ(output, expectOutput);
}
*/


} // namespace facebook::fboss
