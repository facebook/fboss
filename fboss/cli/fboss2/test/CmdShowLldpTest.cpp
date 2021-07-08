// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/lldp/CmdShowLldp.h"
#include "fboss/cli/fboss2/commands/show/lldp/gen-cpp2/model_types.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::vector<facebook::fboss::LinkNeighborThrift> createLldpEntries() {
  facebook::fboss::LinkNeighborThrift lldpEntry1;
  lldpEntry1.localPort_ref() = 102;
  lldpEntry1.systemName_ref() = "fsw001.p001.f01.atn1";
  lldpEntry1.printablePortId_ref() = "Ethernet3/2/1";

  fboss::LinkNeighborThrift lldpEntry2;
  lldpEntry2.localPort_ref() = 106;
  lldpEntry2.systemName_ref() = "fsw002.p001.f01.atn1";
  lldpEntry2.printablePortId_ref() = "Ethernet3/2/1";

  fboss::LinkNeighborThrift lldpEntry3;
  lldpEntry3.localPort_ref() = 110;
  lldpEntry3.systemName_ref() = "fsw003.p001.f01.atn1";
  lldpEntry3.printablePortId_ref() = "Ethernet3/2/1";

  std::vector<fboss::LinkNeighborThrift> entries{lldpEntry1, lldpEntry2, lldpEntry3};
  return entries;
}

class CmdShowLldpTestFixture : public testing::Test {
 public:
  std::vector<fboss::LinkNeighborThrift> lldpEntries;
  folly::IPAddressV4 hostIp;

  void SetUp() override {
    lldpEntries = createLldpEntries();
    hostIp = folly::IPAddressV4::tryFromString("127.0.0.1").value();
  }
};

TEST_F(CmdShowLldpTestFixture, createModel) {
  auto cmd = CmdShowLldp();
  auto model = cmd.createModel(lldpEntries);
  auto entries = model.get_lldpEntries();

  EXPECT_EQ(entries.size(), 3);

  EXPECT_EQ(entries[0].get_localPort(), "102");
  EXPECT_EQ(entries[0].get_systemName(), "fsw001.p001.f01.atn1");
  EXPECT_EQ(entries[0].get_remotePort(), "Ethernet3/2/1");

  EXPECT_EQ(entries[1].get_localPort(), "106");
  EXPECT_EQ(entries[1].get_systemName(), "fsw002.p001.f01.atn1");
  EXPECT_EQ(entries[1].get_remotePort(), "Ethernet3/2/1");

  EXPECT_EQ(entries[2].get_localPort(), "110");
  EXPECT_EQ(entries[2].get_systemName(), "fsw003.p001.f01.atn1");
  EXPECT_EQ(entries[2].get_remotePort(), "Ethernet3/2/1");
}

TEST_F(CmdShowLldpTestFixture, printOutput) {
  auto cmd = CmdShowLldp();
  auto model = cmd.createModel(lldpEntries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
"Local Port       Name                                  Port        \n"
"--------------------------------------------------------------------------------\n"
"102              fsw001.p001.f01.atn1                  Ethernet3/2/1\n"
"106              fsw002.p001.f01.atn1                  Ethernet3/2/1\n"
"110              fsw003.p001.f01.atn1                  Ethernet3/2/1\n\n";
  EXPECT_EQ(output, expectOutput);
}


} // namespace facebook::fboss
