// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include <thrift/lib/cpp2/reflection/testing.h>
#include "fboss/cli/fboss2/commands/show/host/CmdShowHost.h"
#include "fboss/cli/fboss2/commands/show/host/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

// Test IP addresses - using arbitrary addresses since DNS is mocked
const std::string kTestIpv6_1 = "2001:db8::1";
const std::string kTestIpv6_2 = "2001:db8::2";

// Expected hostnames for our mock resolver
const std::string kTestHostname1 = "test-host-1.example.com";
const std::string kTestHostname2 = "test-host-2.example.com";

// Mock DNS resolver that returns deterministic hostnames
std::string mockDnsResolver(const std::string& ipAddr) {
  if (ipAddr == kTestIpv6_1) {
    return kTestHostname1;
  } else if (ipAddr == kTestIpv6_2) {
    return kTestHostname2;
  }
  throw std::runtime_error(
      fmt::format("Unexpected IP address in mock resolver: {}", ipAddr));
}

std::vector<NdpEntryThrift> createMockNdpEntries() {
  NdpEntryThrift ndpEntry1;
  folly::IPAddressV6 ipv6_1(kTestIpv6_1);
  network::thrift::BinaryAddress binaryAddr1 =
      facebook::network::toBinaryAddress(ipv6_1);

  ndpEntry1.ip() = binaryAddr1;
  ndpEntry1.port() = 106;
  ndpEntry1.classID() = 32;

  NdpEntryThrift ndpEntry2;
  folly::IPAddressV6 ipv6_2(kTestIpv6_2);
  network::thrift::BinaryAddress binaryAddr2 =
      facebook::network::toBinaryAddress(ipv6_2);

  ndpEntry2.ip() = binaryAddr2;
  ndpEntry2.port() = 1;
  ndpEntry2.classID() = 0;

  std::vector<NdpEntryThrift> entries{ndpEntry1, ndpEntry2};
  return entries;
}

std::map<int32_t, PortInfoThrift> createMockPortInfoEntries() {
  std::map<int32_t, PortInfoThrift> portInfoMap;

  PortInfoThrift portInfoEntry1;
  portInfoEntry1.portId() = 1;
  portInfoEntry1.name() = "eth4/3/1";
  portInfoEntry1.speedMbps() = 200000;
  portInfoEntry1.fecMode() = "RS528";
  portInfoEntry1.input()->errors()->errors() = 10;
  portInfoEntry1.input()->errors()->discards() = 43;
  portInfoEntry1.output()->errors()->errors() = 2;
  portInfoEntry1.output()->errors()->discards() = 98;

  PortInfoThrift portInfoEntry2;
  portInfoEntry2.portId() = 106;
  portInfoEntry2.name() = "eth5/5/1";
  portInfoEntry2.speedMbps() = 549000;
  portInfoEntry2.fecMode() = "RS544_2N";
  portInfoEntry2.input()->errors()->errors() = 56;
  portInfoEntry2.input()->errors()->discards() = 72;
  portInfoEntry2.output()->errors()->errors() = 12;
  portInfoEntry2.output()->errors()->discards() = 9;

  portInfoMap[folly::copy(portInfoEntry1.portId().value())] = portInfoEntry1;
  portInfoMap[folly::copy(portInfoEntry2.portId().value())] = portInfoEntry2;
  return portInfoMap;
}

std::map<int32_t, PortStatus> createMockPortStatusEntries() {
  std::map<int32_t, PortStatus> portStatusMap;

  PortStatus portStatusEntry1;
  portStatusEntry1.enabled() = true;
  portStatusEntry1.up() = false;

  PortStatus portStatusEntry2;
  portStatusEntry2.enabled() = false;
  portStatusEntry2.up() = true;

  portStatusMap[1] = portStatusEntry1;
  portStatusMap[106] = portStatusEntry2;
  return portStatusMap;
}

cli::ShowHostModel createSortedHostModel() {
  cli::ShowHostModel model;

  cli::ShowHostModelEntry entry1, entry2;
  entry1.portName() = "eth4/3/1";
  entry1.portID() = 1;
  entry1.queueID() = "Olympic";
  entry1.hostName() = kTestHostname2;
  entry1.adminState() = "Enabled";
  entry1.linkState() = "Down";
  entry1.speed() = "200G";
  entry1.fecMode() = "RS528";
  entry1.inErrors() = 10;
  entry1.inDiscards() = 43;
  entry1.outErrors() = 2;
  entry1.outDiscards() = 98;

  entry2.portName() = "eth5/5/1";
  entry2.portID() = 106;
  entry2.queueID() = "22";
  entry2.hostName() = kTestHostname1;
  entry2.adminState() = "Disabled";
  entry2.linkState() = "Up";
  entry2.speed() = "549G";
  entry2.fecMode() = "RS544_2N";
  entry2.inErrors() = 56;
  entry2.inDiscards() = 72;
  entry2.outErrors() = 12;
  entry2.outDiscards() = 9;

  model.hostEntries() = {entry1, entry2};
  return model;
}

class CmdShowHostTestFixture : public CmdHandlerTestBase {
 public:
  CmdShowHostTraits::ObjectArgType queriedPorts;
  std::vector<NdpEntryThrift> mockNdpEntries;
  std::map<int32_t, PortInfoThrift> mockPortInfoEntries;
  std::map<int32_t, PortStatus> mockPortStatusEntries;
  cli::ShowHostModel expectedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockNdpEntries = createMockNdpEntries();
    mockPortInfoEntries = createMockPortInfoEntries();
    mockPortStatusEntries = createMockPortStatusEntries();
    expectedModel = createSortedHostModel();
  }
};

TEST_F(CmdShowHostTestFixture, sortModel) {
  auto model = CmdShowHost().createModel(
      mockNdpEntries,
      mockPortInfoEntries,
      mockPortStatusEntries,
      queriedPorts,
      mockDnsResolver);

  EXPECT_THRIFT_EQ(expectedModel, model);
}

TEST_F(CmdShowHostTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getNdpTable(_))
      .WillOnce(Invoke([&](auto& entries) { entries = mockNdpEntries; }));
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = mockPortInfoEntries; }));
  EXPECT_CALL(getMockAgent(), getPortStatus(_, _))
      .WillOnce(Invoke(
          [&](auto& entries, auto) { entries = mockPortStatusEntries; }));

  auto cmd = CmdShowHost();
  auto model = cmd.queryClient(localhost(), queriedPorts, mockDnsResolver);

  EXPECT_THRIFT_EQ(expectedModel, model);
}

TEST_F(CmdShowHostTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowHost().printOutput(expectedModel, ss);

  std::string output = ss.str();
  std::string expectOutput =
      " Port      ID   Queue ID  Hostname                 Admin State  Link State  Speed  FEC       InErr  InDiscard  OutErr  OutDiscard \n"
      "-----------------------------------------------------------------------------------------------------------------------------------------------\n"
      " eth4/3/1  1    Olympic   test-host-2.example.com  Enabled      Down        200G   RS528     10     43         2       98         \n"
      " eth5/5/1  106  22        test-host-1.example.com  Disabled     Up          549G   RS544_2N  56     72         12      9          \n\n";

  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
