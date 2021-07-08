// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/ndp/CmdShowNdp.h"
#include "fboss/cli/fboss2/commands/show/ndp/gen-cpp2/model_types.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::vector<facebook::fboss::NdpEntryThrift> createNdpEntries() {
  facebook::fboss::NdpEntryThrift ndpEntry1;
  folly::IPAddressV6 ipv6_1("fe80::526b:4bff:fe28:8fb0");
  network::thrift::BinaryAddress binaryAddr1 =
      facebook::network::toBinaryAddress(ipv6_1);

  ndpEntry1.ip_ref() = binaryAddr1;
  ndpEntry1.mac_ref() = "50:6b:4b:28:8f:b0";
  ndpEntry1.port_ref() = 46;
  ndpEntry1.vlanName_ref() = "downlinks";
  ndpEntry1.vlanID_ref() = 2000;
  ndpEntry1.state_ref() = "REACHABLE";
  ndpEntry1.ttl_ref() = 45013;
  ndpEntry1.classID_ref() = 0;

  fboss::NdpEntryThrift ndpEntry2;
  folly::IPAddressV6 ipv6_2("fe80::464c:a8ff:fee4:1c3f");
  network::thrift::BinaryAddress binaryAddr2 =
      facebook::network::toBinaryAddress(ipv6_2);

  ndpEntry2.ip_ref() = binaryAddr2;
  ndpEntry2.mac_ref() = "44:4c:a8:e4:1c:3f";
  ndpEntry2.port_ref() = 102;
  ndpEntry2.vlanName_ref() = "uplink_1";
  ndpEntry2.vlanID_ref() = 4001;
  ndpEntry2.state_ref() = "REACHABLE";
  ndpEntry2.ttl_ref() = 21045;
  ndpEntry2.classID_ref() = 0;

  std::vector<fboss::NdpEntryThrift> entries{ndpEntry1, ndpEntry2};
  return entries;
}

class CmdShowNdpTestFixture : public testing::Test {
 public:
  std::vector<fboss::NdpEntryThrift> ndpEntries;
  folly::IPAddressV4 hostIp;

  void SetUp() override {
    ndpEntries = createNdpEntries();
    hostIp = folly::IPAddressV4::tryFromString("127.0.0.1").value();
  }
};

TEST_F(CmdShowNdpTestFixture, createModel) {
  auto cmd = CmdShowNdp();
  CmdShowNdpTraits::ObjectArgType queriedEntries;
  auto model = cmd.createModel(ndpEntries, queriedEntries);
  auto entries = model.get_ndpEntries();

  EXPECT_EQ(entries.size(), 2);

  EXPECT_EQ(entries[0].get_ip(), "fe80::526b:4bff:fe28:8fb0");
  EXPECT_EQ(entries[0].get_mac(), "50:6b:4b:28:8f:b0");
  EXPECT_EQ(entries[0].get_port(), 46);
  EXPECT_EQ(entries[0].get_vlanName(), "downlinks");
  EXPECT_EQ(entries[0].get_vlanID(), 2000);
  EXPECT_EQ(entries[0].get_state(), "REACHABLE");
  EXPECT_EQ(entries[0].get_ttl(), 45013);
  EXPECT_EQ(entries[0].get_classID(), 0);

  EXPECT_EQ(entries[1].get_ip(), "fe80::464c:a8ff:fee4:1c3f");
  EXPECT_EQ(entries[1].get_mac(), "44:4c:a8:e4:1c:3f");
  EXPECT_EQ(entries[1].get_port(), 102);
  EXPECT_EQ(entries[1].get_vlanName(), "uplink_1");
  EXPECT_EQ(entries[1].get_vlanID(), 4001);
  EXPECT_EQ(entries[1].get_state(), "REACHABLE");
  EXPECT_EQ(entries[1].get_ttl(), 21045);
  EXPECT_EQ(entries[1].get_classID(), 0);

}

TEST_F(CmdShowNdpTestFixture, printOutput) {
  auto cmd = CmdShowNdp();
  CmdShowNdpTraits::ObjectArgType queriedEntries;
  auto model = cmd.createModel(ndpEntries, queriedEntries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
"IP Address                                   MAC Address        Port        VLAN               State         TTL      CLASSID     \n"
"fe80::526b:4bff:fe28:8fb0                    50:6b:4b:28:8f:b0  46          downlinks (2000)   REACHABLE     45013    0           \n"
"fe80::464c:a8ff:fee4:1c3f                    44:4c:a8:e4:1c:3f  102         uplink_1 (4001)    REACHABLE     21045    0           \n\n";
  EXPECT_EQ(output, expectOutput);
}


} // namespace facebook::fboss
