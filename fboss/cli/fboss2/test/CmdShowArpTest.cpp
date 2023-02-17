// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>
#include <cstdint>

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/commands/show/arp/CmdShowArp.h"
#include "fboss/cli/fboss2/commands/show/arp/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

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

  arpEntry1.ip() = binaryAddr1;
  arpEntry1.mac() = "44:4c:a8:e4:1c:3f";
  arpEntry1.port() = 102;
  arpEntry1.vlanName() = "vlan4001";
  arpEntry1.vlanID() = 4001;
  arpEntry1.state() = "REACHABLE";
  arpEntry1.ttl() = 27198;
  arpEntry1.classID() = 0;

  fboss::ArpEntryThrift arpEntry2;
  folly::IPAddressV4 ip2("10.121.64.2");
  network::thrift::BinaryAddress binaryAddr2 =
      facebook::network::toBinaryAddress(ip2);

  arpEntry2.ip() = binaryAddr2;
  arpEntry2.mac() = "44:4c:a8:e4:1b:f1";
  arpEntry2.port() = 106;
  arpEntry2.vlanName() = "vlan4002";
  arpEntry2.vlanID() = 4002;
  arpEntry2.state() = "REACHABLE";
  arpEntry2.ttl() = 33730;
  arpEntry2.classID() = 0;

  std::vector<fboss::ArpEntryThrift> entries{arpEntry1, arpEntry2};
  return entries;
}

std::map<int32_t, facebook::fboss::PortInfoThrift>
createFakePortThriftEntries() {
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;

  PortInfoThrift portEntry1;
  portEntry1.portId() = 102;
  portEntry1.name() = "eth1/1/1";

  PortInfoThrift portEntry2;
  portEntry2.portId() = 106;
  portEntry2.name() = "eth2/1/1";

  portEntries[portEntry1.get_portId()] = portEntry1;
  portEntries[portEntry2.get_portId()] = portEntry2;

  return portEntries;
}

class CmdShowArpTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<fboss::ArpEntryThrift> arpEntries;
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    arpEntries = createArpEntries();
    portEntries = createFakePortThriftEntries();
  }
};

TEST_F(CmdShowArpTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getArpTable(_))
      .WillOnce(Invoke([&](auto& entries) { entries = arpEntries; }));

  auto cmd = CmdShowArp();
  auto result = cmd.queryClient(localhost());
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
  auto model = cmd.createModel(arpEntries, portEntries, {});

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::cout << output;
}

} // namespace facebook::fboss
