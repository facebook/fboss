// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/arp/CmdShowArp.h"
#include "fboss/cli/fboss2/commands/show/arp/gen-cpp2/model_types.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::vector<facebook::fboss::ArpEntryThrift> createArpEntries() {
  facebook::fboss::ArpEntryThrift arpEntry1;
  folly::IPAddressV4 ip1("10.120.64.2");
  network::thrift::BinaryAddress binaryAddr1 =
      facebook::network::toBinaryAddress(ip1);

  arpEntry1.ip_ref() = binaryAddr1;
  arpEntry1.mac_ref() = "44:4c:a8:e4:1c:3f";
  arpEntry1.port_ref() = 102;
  arpEntry1.vlanName_ref() = "vlan4001";
  arpEntry1.vlanID_ref() = 4001;
  arpEntry1.state_ref() = "REACHABLE";
  arpEntry1.ttl_ref() = 27198;
  arpEntry1.classID_ref() = 0;

  fboss::ArpEntryThrift arpEntry2;
  folly::IPAddressV4 ip2("10.121.64.2");
  network::thrift::BinaryAddress binaryAddr2 =
      facebook::network::toBinaryAddress(ip2);

  arpEntry2.ip_ref() = binaryAddr2;
  arpEntry2.mac_ref() = "44:4c:a8:e4:1b:f1";
  arpEntry2.port_ref() = 106;
  arpEntry2.vlanName_ref() = "vlan4002";
  arpEntry2.vlanID_ref() = 4002;
  arpEntry2.state_ref() = "REACHABLE";
  arpEntry2.ttl_ref() = 33730;
  arpEntry2.classID_ref() = 0;

  std::vector<fboss::ArpEntryThrift> entries{arpEntry1, arpEntry2};
  return entries;
}

class CmdShowArpTestFixture : public testing::Test {
 public:
  std::vector<fboss::ArpEntryThrift> arpEntries;
  folly::IPAddressV4 hostIp;

  void SetUp() override {
    arpEntries = createArpEntries();
    hostIp = folly::IPAddressV4::tryFromString("127.0.0.1").value();
  }
};

TEST_F(CmdShowArpTestFixture, queryClient) {
  auto cmd = CmdShowArp();
  auto result = cmd.queryClient(hostIp);
  auto entries = result.get_arpEntries();
  EXPECT_EQ(entries.size(), 2);

  EXPECT_EQ(entries[0].get_ip(), "10.120.64.2");
  EXPECT_EQ(entries[0].get_mac(), "44:4c:a8:e4:1c:3f");
  EXPECT_EQ(entries[0].get_port(), 102);
  EXPECT_EQ(entries[0].get_state(), "REACHABLE");

  EXPECT_EQ(entries[1].get_ip(), "10.121.64.2");
  EXPECT_EQ(entries[1].get_mac(), "44:4c:a8:e4:1b:f1");
  EXPECT_EQ(entries[1].get_port(), 106);
  EXPECT_EQ(entries[1].get_state(), "REACHABLE");
}

TEST_F(CmdShowArpTestFixture, printOutput) {
  auto cmd = CmdShowArp();
  auto model = cmd.createModel(arpEntries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
      "IP Address            MAC Address        Port        VLAN               State         TTL      CLASSID     \n10.120.64.2           44:4c:a8:e4:1c:3f  102         vlan4001 (4001)    REACHABLE     27198    0           \n10.121.64.2           44:4c:a8:e4:1b:f1  106         vlan4002 (4002)    REACHABLE     33730    0           \n\n";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
