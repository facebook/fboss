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
  lldpEntry1.localPort() = 102;
  lldpEntry1.systemName() = "fsw001.p001.f01.atn1";
  lldpEntry1.printablePortId() = "Ethernet3/2/1";
  lldpEntry1.localPortName() = "eth1/1/1";
  lldpEntry1.systemDescription() = "FBOSS";

  fboss::LinkNeighborThrift lldpEntry2;
  lldpEntry2.localPort() = 106;
  lldpEntry2.systemName() = "fsw002.p001.f01.atn1";
  lldpEntry2.printablePortId() = "Ethernet3/2/1";
  lldpEntry2.localPortName() = "eth2/1/1";
  lldpEntry2.systemDescription() = "FBOSS";

  fboss::LinkNeighborThrift lldpEntry3;
  lldpEntry3.localPort() = 110;
  lldpEntry3.systemName() = "fsw003.p001.f01.atn1";
  lldpEntry3.printablePortId() = "Ethernet3/2/1";
  lldpEntry3.localPortName() = "eth3/1/1";
  lldpEntry3.systemDescription() = "FBOSS";

  fboss::LinkNeighborThrift lldpEntry4;
  lldpEntry4.localPort() = 114;
  lldpEntry4.systemName() = "fsw004.p001.f01.atn1";
  lldpEntry4.printablePortId() = "Ethernet3/2/1";
  lldpEntry4.localPortName() = "eth3/2/1";
  lldpEntry4.systemDescription() = "FBOSS";

  fboss::LinkNeighborThrift lldpEntry5;
  lldpEntry5.localPort() = 118;
  lldpEntry5.systemName() = "fsw005.p001.f01.atn1";
  lldpEntry5.printablePortId() = "Ethernet3/2/1";
  lldpEntry5.localPortName() = "eth3/10/1";
  lldpEntry5.systemDescription() = "FBOSS";

  std::vector<fboss::LinkNeighborThrift> entries{
      lldpEntry1, lldpEntry2, lldpEntry3, lldpEntry4, lldpEntry5};
  return entries;
}

std::map<int32_t, facebook::fboss::PortInfoThrift> createPortInfo() {
  facebook::fboss::PortInfoThrift entry1;
  entry1.portId() = 102;
  entry1.operState() = facebook::fboss::PortOperState::UP;
  entry1.description() = "u-001: fsw001.p001 (F=spine:L=d-051)";

  facebook::fboss::PortInfoThrift entry2;
  entry2.portId() = 106;
  entry2.operState() = facebook::fboss::PortOperState::UP;
  entry2.description() = "u-001: fsw002.p001 (F=spine:L=d-051)";

  facebook::fboss::PortInfoThrift entry3;
  entry3.portId() = 110;
  entry3.operState() = facebook::fboss::PortOperState::UP;
  entry3.description() = "u-001: fsw003.p001 (F=spine:L=d-051)";

  facebook::fboss::PortInfoThrift entry4;
  entry4.portId() = 114;
  entry4.operState() = facebook::fboss::PortOperState::UP;
  entry4.description() = "u-001: fsw004.p001 (F=spine:L=d-051)";

  facebook::fboss::PortInfoThrift entry5;
  entry5.portId() = 118;
  entry5.operState() = facebook::fboss::PortOperState::UP;
  entry5.description() = "u-001: fsw005.p001 (F=spine:L=d-051)";

  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  portEntries.insert(std::make_pair(1, entry1));
  portEntries.insert(std::make_pair(2, entry2));
  portEntries.insert(std::make_pair(3, entry3));
  portEntries.insert(std::make_pair(4, entry4));
  portEntries.insert(std::make_pair(5, entry5));

  return portEntries;
}

class CmdShowLldpTestFixture : public testing::Test {
 public:
  std::vector<fboss::LinkNeighborThrift> lldpEntries;
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  std::vector<std::string> queriedIfs;
  folly::IPAddressV4 hostIp;

  void SetUp() override {
    lldpEntries = createLldpEntries();
    portEntries = createPortInfo();
    hostIp = folly::IPAddressV4::tryFromString("127.0.0.1").value();
  }
};

TEST_F(CmdShowLldpTestFixture, createModel) {
  auto cmd = CmdShowLldp();
  auto model = cmd.createModel(lldpEntries, portEntries, queriedIfs);
  auto entries = model.get_lldpEntries();

  EXPECT_EQ(entries.size(), 5);

  EXPECT_EQ(entries[0].get_localPort(), "eth1/1/1");
  EXPECT_EQ(entries[0].get_systemName(), "fsw001.p001.f01.atn1");
  EXPECT_EQ(entries[0].get_remotePort(), "Ethernet3/2/1");

  EXPECT_EQ(entries[1].get_localPort(), "eth2/1/1");
  EXPECT_EQ(entries[1].get_systemName(), "fsw002.p001.f01.atn1");
  EXPECT_EQ(entries[1].get_remotePort(), "Ethernet3/2/1");

  EXPECT_EQ(entries[2].get_localPort(), "eth3/1/1");
  EXPECT_EQ(entries[2].get_systemName(), "fsw003.p001.f01.atn1");
  EXPECT_EQ(entries[2].get_remotePort(), "Ethernet3/2/1");
}

TEST_F(CmdShowLldpTestFixture, printOutput) {
  auto cmd = CmdShowLldp();
  auto model = cmd.createModel(lldpEntries, portEntries, queriedIfs);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
      " Local Int  Status  Expected-Peer  LLDP-Peer             Peer-Int       Platform  Peer Description                     \n"
      "-------------------------------------------------------------------------------------------------------------------------------\n"
      " eth1/1/1   up      fsw001.p001    fsw001.p001.f01.atn1  Ethernet3/2/1  FBOSS     u-001: fsw001.p001 (F=spine:L=d-051) \n"
      " eth2/1/1   up      fsw002.p001    fsw002.p001.f01.atn1  Ethernet3/2/1  FBOSS     u-001: fsw002.p001 (F=spine:L=d-051) \n"
      " eth3/1/1   up      fsw003.p001    fsw003.p001.f01.atn1  Ethernet3/2/1  FBOSS     u-001: fsw003.p001 (F=spine:L=d-051) \n"
      " eth3/2/1   up      fsw004.p001    fsw004.p001.f01.atn1  Ethernet3/2/1  FBOSS     u-001: fsw004.p001 (F=spine:L=d-051) \n"
      " eth3/10/1  up      fsw005.p001    fsw005.p001.f01.atn1  Ethernet3/2/1  FBOSS     u-001: fsw005.p001 (F=spine:L=d-051) \n\n";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
