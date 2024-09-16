// (c) Meta Platforms, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/commands/show/interface/CmdShowInterface.h"
#include "fboss/cli/fboss2/commands/show/interface/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdShowInterfaceTestFixture : public CmdHandlerTestBase {
 public:
  HostInfo hostInfo{"hostName"};
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  std::map<int32_t, facebook::fboss::InterfaceDetail> intfDetails;
  std::map<int64_t, cfg::DsfNode> dsfNodes;
  std::vector<std::string> queriedEntries;
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    portEntries = createPortEntries();
    intfDetails = createInterfaceDetails();
    dsfNodes = {};
  }

  std::map<int32_t, facebook::fboss::PortInfoThrift> createPortEntries() {
    std::map<int32_t, facebook::fboss::PortInfoThrift> portMap;

    facebook::fboss::PortInfoThrift portEntry1;
    portEntry1.portId() = 1;
    portEntry1.name() = "eth1/1/1";
    portEntry1.description() = "d-021: twshared54324.lol2";
    portEntry1.operState() = facebook::fboss::PortOperState::UP;
    portEntry1.vlans() = {2001};
    portEntry1.speedMbps() = 100000;

    facebook::fboss::PortInfoThrift portEntry2;
    portEntry2.portId() = 2;
    portEntry2.name() = "eth2/1/1";
    portEntry2.description() = "d-022: gpuhost1234.lol2";
    portEntry2.operState() = facebook::fboss::PortOperState::UP;
    portEntry2.vlans() = {2002};
    portEntry2.speedMbps() = 200000;

    facebook::fboss::PortInfoThrift portEntry3;
    portEntry3.portId() = 3;
    portEntry3.name() = "eth3/1/1";
    portEntry3.description() = "ctsw001.c081.f00.lol1:Ethernet5/4/1";
    portEntry3.operState() = facebook::fboss::PortOperState::DOWN;
    portEntry3.vlans() = {4001};
    portEntry3.speedMbps() = 400000;

    portMap[*portEntry1.portId()] = portEntry1;
    portMap[*portEntry2.portId()] = portEntry2;
    portMap[*portEntry3.portId()] = portEntry3;

    return portMap;
  }

  std::map<int32_t, facebook::fboss::InterfaceDetail> createInterfaceDetails() {
    std::map<int32_t, facebook::fboss::InterfaceDetail> intfDetails;

    facebook::fboss::InterfaceDetail intfDetail1;
    intfDetail1.interfaceName() = "downlink_1";
    intfDetail1.vlanId() = 2001;
    intfDetail1.mtu() = 1500;
    intfDetail1.address() = createIpPrefixesForIntf1();

    facebook::fboss::InterfaceDetail intfDetail2;
    intfDetail2.interfaceName() = "downlink_2";
    intfDetail2.vlanId() = 2002;
    intfDetail2.mtu() = 1500;
    intfDetail2.address() = createIpPrefixesForIntf2();

    facebook::fboss::InterfaceDetail intfDetail3;
    intfDetail3.interfaceName() = "uplink_1";
    intfDetail3.vlanId() = 4001;
    intfDetail3.mtu() = 9000;
    intfDetail3.address() = createIpPrefixesForIntf3();

    intfDetails[*intfDetail1.vlanId()] = intfDetail1;
    intfDetails[*intfDetail2.vlanId()] = intfDetail2;
    intfDetails[*intfDetail3.vlanId()] = intfDetail3;

    return intfDetails;
  }

  std::vector<facebook::fboss::IpPrefix> createIpPrefixesForIntf1() {
    facebook::fboss::IpPrefix prefix1;
    prefix1.ip() =
        facebook::network::toBinaryAddress(folly::IPAddressV4("10.1.1.1"));
    prefix1.prefixLength() = 31;

    facebook::fboss::IpPrefix prefix2;
    prefix2.ip() = facebook::network::toBinaryAddress(
        folly::IPAddressV6("2401:db00::0001:abcd"));
    prefix2.prefixLength() = 127;

    facebook::fboss::IpPrefix prefix3;
    prefix3.ip() = facebook::network::toBinaryAddress(
        folly::IPAddressV6("fe80::be:face:b00c"));
    prefix3.prefixLength() = 64;

    return {prefix1, prefix2, prefix3};
  }

  std::vector<facebook::fboss::IpPrefix> createIpPrefixesForIntf2() {
    facebook::fboss::IpPrefix prefix1;
    prefix1.ip() =
        facebook::network::toBinaryAddress(folly::IPAddressV4("10.1.2.1"));
    prefix1.prefixLength() = 31;

    facebook::fboss::IpPrefix prefix2;
    prefix2.ip() = facebook::network::toBinaryAddress(
        folly::IPAddressV6("2401:db00::0002:abcd"));
    prefix2.prefixLength() = 127;

    facebook::fboss::IpPrefix prefix3;
    prefix3.ip() = facebook::network::toBinaryAddress(
        folly::IPAddressV6("fe80::be:face:b00c"));
    prefix3.prefixLength() = 64;

    return {prefix1, prefix2, prefix3};
  }

  std::vector<facebook::fboss::IpPrefix> createIpPrefixesForIntf3() {
    facebook::fboss::IpPrefix prefix1;
    prefix1.ip() = facebook::network::toBinaryAddress(
        folly::IPAddressV6("2401:db00::0003:abcd"));
    prefix1.prefixLength() = 127;

    facebook::fboss::IpPrefix prefix2;
    prefix2.ip() = facebook::network::toBinaryAddress(
        folly::IPAddressV6("fe80::be:face:b00c"));
    prefix2.prefixLength() = 64;

    return {prefix1, prefix2};
  }
};

TEST_F(CmdShowInterfaceTestFixture, createModel) {
  auto cmd = CmdShowInterface();
  auto model = cmd.createModel(
      hostInfo, portEntries, intfDetails, dsfNodes, queriedEntries);
  auto intfAddressModel = *model.interfaces();

  EXPECT_EQ(intfAddressModel.size(), 3);

  EXPECT_EQ(*intfAddressModel[0].name(), "eth1/1/1");
  EXPECT_EQ(*intfAddressModel[0].speed(), "100G");
  ASSERT_TRUE(intfAddressModel[0].vlan());
  EXPECT_EQ(*intfAddressModel[0].vlan(), 2001);
  EXPECT_EQ(*intfAddressModel[0].mtu(), 1500);
  EXPECT_EQ(*intfAddressModel[0].prefixes()[0].ip(), "10.1.1.1");
  EXPECT_EQ(*intfAddressModel[0].prefixes()[0].prefixLength(), 31);
  EXPECT_EQ(*intfAddressModel[0].prefixes()[1].ip(), "2401:db00::1:abcd");
  EXPECT_EQ(*intfAddressModel[0].prefixes()[1].prefixLength(), 127);
  EXPECT_EQ(*intfAddressModel[0].prefixes()[2].ip(), "fe80::be:face:b00c");
  EXPECT_EQ(*intfAddressModel[0].prefixes()[2].prefixLength(), 64);

  EXPECT_EQ(*intfAddressModel[1].name(), "eth2/1/1");
  EXPECT_EQ(*intfAddressModel[1].speed(), "200G");
  ASSERT_TRUE(intfAddressModel[1].vlan());
  EXPECT_EQ(*intfAddressModel[1].vlan(), 2002);
  EXPECT_EQ(*intfAddressModel[1].mtu(), 1500);
  EXPECT_EQ(*intfAddressModel[1].prefixes()[0].ip(), "10.1.2.1");
  EXPECT_EQ(*intfAddressModel[1].prefixes()[0].prefixLength(), 31);
  EXPECT_EQ(*intfAddressModel[1].prefixes()[1].ip(), "2401:db00::2:abcd");
  EXPECT_EQ(*intfAddressModel[1].prefixes()[1].prefixLength(), 127);
  EXPECT_EQ(*intfAddressModel[1].prefixes()[2].ip(), "fe80::be:face:b00c");
  EXPECT_EQ(*intfAddressModel[1].prefixes()[2].prefixLength(), 64);

  EXPECT_EQ(*intfAddressModel[2].name(), "eth3/1/1");
  EXPECT_EQ(*intfAddressModel[2].speed(), "400G");
  ASSERT_TRUE(intfAddressModel[2].vlan());
  EXPECT_EQ(*intfAddressModel[2].vlan(), 4001);
  EXPECT_EQ(*intfAddressModel[2].mtu(), 9000);
  EXPECT_EQ(*intfAddressModel[2].prefixes()[0].ip(), "2401:db00::3:abcd");
  EXPECT_EQ(*intfAddressModel[2].prefixes()[0].prefixLength(), 127);
  EXPECT_EQ(*intfAddressModel[2].prefixes()[1].ip(), "fe80::be:face:b00c");
  EXPECT_EQ(*intfAddressModel[2].prefixes()[1].prefixLength(), 64);
}

TEST_F(CmdShowInterfaceTestFixture, printOutput) {
  auto cmd = CmdShowInterface();
  auto model = cmd.createModel(
      hostInfo, portEntries, intfDetails, dsfNodes, queriedEntries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      "+-----------+--------+-------+------+------+-----------------------+-------------------------------------+\n"
      "| Interface | Status | Speed | VLAN | MTU  | Addresses             | Description                         |\n"
      "----------------------------------------------------------------------------------------------------------\n"
      "| eth1/1/1  | up     | 100G  | 2001 | 1500 | 10.1.1.1/31           | d-021: twshared54324.lol2           |\n"
      "|           |        |       |      |      | 2401:db00::1:abcd/127 |                                     |\n"
      "|           |        |       |      |      | fe80::be:face:b00c/64 |                                     |\n"
      "+-----------+--------+-------+------+------+-----------------------+-------------------------------------+\n"
      "| eth2/1/1  | up     | 200G  | 2002 | 1500 | 10.1.2.1/31           | d-022: gpuhost1234.lol2             |\n"
      "|           |        |       |      |      | 2401:db00::2:abcd/127 |                                     |\n"
      "|           |        |       |      |      | fe80::be:face:b00c/64 |                                     |\n"
      "+-----------+--------+-------+------+------+-----------------------+-------------------------------------+\n"
      "| eth3/1/1  | down   | 400G  | 4001 | 9000 | 2401:db00::3:abcd/127 | ctsw001.c081.f00.lol1:Ethernet5/4/1 |\n"
      "|           |        |       |      |      | fe80::be:face:b00c/64 |                                     |\n"
      "+-----------+--------+-------+------+------+-----------------------+-------------------------------------+\n";

  EXPECT_EQ(output, expectedOutput);
}
} // namespace facebook::fboss
