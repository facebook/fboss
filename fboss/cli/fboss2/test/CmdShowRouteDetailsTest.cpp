// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteDetails.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::vector<RouteDetails> createRouteEntries() {
  std::vector<RouteDetails> routeEntries;
  RouteDetails routeEntry1, routeEntry2;

  // routeEntry1
  folly::IPAddressV6 ip1_1("2401:db00::");
  network::thrift::BinaryAddress binaryAddr1 =
      facebook::network::toBinaryAddress(ip1_1);

  IpPrefix ipPrefix1;
  ipPrefix1.ip() = binaryAddr1;
  ipPrefix1.prefixLength() = 32;
  routeEntry1.dest() = ipPrefix1;

  folly::IPAddressV6 ip1_2("2401:db00:e32f:8fc::2");
  network::thrift::BinaryAddress binaryAddr1_2 =
      facebook::network::toBinaryAddress(ip1_2);

  MplsAction mplsAction1_1;
  mplsAction1_1.action() = MplsActionCode::SWAP;
  mplsAction1_1.swapLabel() = 1;

  NextHopThrift nextHop1_1;
  nextHop1_1.address() = binaryAddr1_2;
  nextHop1_1.weight() = 1;
  nextHop1_1.mplsAction() = mplsAction1_1;

  ClientAndNextHops clAndNxthops1;
  clAndNxthops1.clientId() = 0;
  clAndNxthops1.nextHops()->emplace_back(nextHop1_1);
  routeEntry1.nextHopMulti()->emplace_back(clAndNxthops1);

  routeEntry1.action() = "Nexthops";
  routeEntry1.isConnected() = false;

  network::thrift::BinaryAddress binaryAddr1_3 =
      facebook::network::toBinaryAddress(ip1_2);
  binaryAddr1_3.ifName() = "Port-Channel304";

  MplsAction mplsAction1_2;
  mplsAction1_2.action() = MplsActionCode::PUSH;
  std::vector<MplsLabel> pushLabels = {2, 3};
  mplsAction1_2.pushLabels() = pushLabels;

  NextHopThrift nextHop1_2;
  nextHop1_2.address() = binaryAddr1_3;
  nextHop1_2.weight() = 1;
  nextHop1_2.mplsAction() = mplsAction1_2;
  routeEntry1.nextHops()->emplace_back(nextHop1_2);
  routeEntry1.classID() = cfg::AclLookupClass::DST_CLASS_L3_DPR;

  // routeEntry2
  folly::IPAddressV4 ip2_1("176.161.6.0");
  network::thrift::BinaryAddress binaryAddr2_1 =
      facebook::network::toBinaryAddress(ip2_1);

  IpPrefix ipPrefix2;
  ipPrefix2.ip() = binaryAddr2_1;
  ipPrefix2.prefixLength() = 32;
  routeEntry2.dest() = ipPrefix2;

  folly::IPAddressV4 ip2_2("240.161.6.0");
  network::thrift::BinaryAddress binaryAddr2_2 =
      facebook::network::toBinaryAddress(ip2_2);

  IfAndIP ifAndIp2;
  ifAndIp2.interfaceID() = 0;
  ifAndIp2.ip() = binaryAddr2_2;
  routeEntry2.fwdInfo()->emplace_back(ifAndIp2);

  ClientAndNextHops clAndNxthops2;
  clAndNxthops2.clientId() = 1;
  clAndNxthops2.nextHopAddrs()->emplace_back(binaryAddr2_2);
  routeEntry2.nextHopMulti()->emplace_back(clAndNxthops2);

  routeEntry2.action() = "Nexthops";
  routeEntry2.isConnected() = true;
  routeEntry2.adminDistance() = AdminDistance::DIRECTLY_CONNECTED;
  routeEntry2.counterID() = "counter0";

  routeEntries.emplace_back(routeEntry1);
  routeEntries.emplace_back(routeEntry2);
  return routeEntries;
}

cli::ShowRouteDetailsModel createRouteModel() {
  cli::ShowRouteDetailsModel model;
  cli::RouteDetailEntry entry1, entry2;

  // entry1
  entry1.ip() = "2401:db00::";
  entry1.prefixLength() = 32;
  entry1.action() = "Nexthops";

  cli::MplsActionInfo mplsActionInfo1_1;
  mplsActionInfo1_1.action() = "SWAP";
  mplsActionInfo1_1.swapLabel() = 1;

  cli::NextHopInfo nextHopInfo1_1;
  nextHopInfo1_1.addr() = "2401:db00:e32f:8fc::2";
  nextHopInfo1_1.weight() = 1;
  nextHopInfo1_1.mplsAction() = mplsActionInfo1_1;

  cli::ClientAndNextHops clnAndNxtHops1;
  clnAndNxtHops1.clientId() = 0;
  clnAndNxtHops1.nextHops()->emplace_back(nextHopInfo1_1);
  entry1.nextHopMulti()->emplace_back(clnAndNxtHops1);

  entry1.isConnected() = false;
  entry1.adminDistance() = "None";

  cli::MplsActionInfo mplsActionInfo1_2;
  mplsActionInfo1_2.action() = "PUSH";
  mplsActionInfo1_2.pushLabels() = {2, 3};

  cli::NextHopInfo nextHopInfo1_2;
  nextHopInfo1_2.addr() = "2401:db00:e32f:8fc::2";
  nextHopInfo1_2.weight() = 1;
  nextHopInfo1_2.mplsAction() = mplsActionInfo1_2;
  nextHopInfo1_2.ifName() = "Port-Channel304";

  entry1.nextHops()->emplace_back(nextHopInfo1_2);
  entry1.counterID() = "None";
  entry1.classID() = "DST_CLASS_L3_DPR(20)";

  // entry 2
  entry2.ip() = "176.161.6.0";
  entry2.prefixLength() = 32;
  entry2.action() = "Nexthops";

  cli::NextHopInfo nextHopInfo2_1;
  nextHopInfo2_1.addr() = "240.161.6.0";
  nextHopInfo2_1.weight() = 0;

  cli::ClientAndNextHops clnAndNxtHops2;
  clnAndNxtHops2.clientId() = 1;
  clnAndNxtHops2.nextHops()->emplace_back(nextHopInfo2_1);
  entry2.nextHopMulti()->emplace_back(clnAndNxtHops2);

  entry2.isConnected() = true;
  entry2.adminDistance() = "DIRECTLY_CONNECTED";

  cli::NextHopInfo nextHopInfo2_2;
  nextHopInfo2_2.addr() = "240.161.6.0";
  nextHopInfo2_2.interfaceID() = 0;

  entry2.nextHops()->emplace_back(nextHopInfo2_2);
  entry2.counterID() = "counter0";
  entry2.classID() = "None";

  model.routeEntries() = {entry1, entry2};

  return model;
}

class CmdShowRouteDetailsTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<facebook::fboss::RouteDetails> routeEntries;
  cli::ShowRouteDetailsModel normalizedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    routeEntries = createRouteEntries();
    normalizedModel = createRouteModel();
  }
};

TEST_F(CmdShowRouteDetailsTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getRouteTableDetails(_))
      .WillOnce(Invoke([&](auto& entries) { entries = routeEntries; }));

  auto cmd = CmdShowRouteDetails();
  CmdShowRouteDetailsTraits::ObjectArgType queriedEntries;
  auto model = cmd.queryClient(localhost(), queriedEntries);

  EXPECT_THRIFT_EQ(model, normalizedModel);
}

TEST_F(CmdShowRouteDetailsTestFixture, queryNetworkEntries) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getRouteTableDetails(_))
      .WillOnce(Invoke([&](auto& entries) { entries = routeEntries; }));

  auto cmd = CmdShowRouteDetails();
  std::vector<std::string> entries = {"2401:db00::/32", "176.161.6.0/32"};
  CmdShowRouteDetailsTraits::ObjectArgType queriedEntries(entries);
  auto model = cmd.queryClient(localhost(), queriedEntries);

  EXPECT_THRIFT_EQ(model, normalizedModel);
}

TEST_F(CmdShowRouteDetailsTestFixture, queryIpRouteEntries) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getRouteTableDetails(_))
      .WillOnce(Invoke([&](auto& entries) { entries = routeEntries; }));
  EXPECT_CALL(getMockAgent(), getIpRouteDetails(_, _, _))
      .WillOnce(
          Invoke([&](auto& entry, auto, auto) { entry = routeEntries[0]; }));

  auto cmd = CmdShowRouteDetails();
  std::vector<std::string> entries = {
      "2401:db00::/32", "176.161.6.0/32", "1.1.1.1"};
  CmdShowRouteDetailsTraits::ObjectArgType queriedEntries(entries);
  auto model = cmd.queryClient(localhost(), queriedEntries);

  EXPECT_THRIFT_EQ(model, normalizedModel);
}

TEST_F(CmdShowRouteDetailsTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowRouteDetails().printOutput(normalizedModel, ss);

  std::string output = ss.str();
  std::string expectOutput = R"(
Network Address: 2401:db00::/32
  Nexthops from client BGPD
    2401:db00:e32f:8fc::2 weight 1 MPLS -> SWAP : 1
  Action: Nexthops
  Forwarding via:
    2401:db00:e32f:8fc::2 dev Port-Channel304 weight 1 MPLS -> PUSH : {2,3}
  Admin Distance: None
  Counter Id: None
  Class Id: DST_CLASS_L3_DPR(20)

Network Address: 176.161.6.0/32 (connected)
  Nexthops from client STATIC_ROUTE
    240.161.6.0
  Action: Nexthops
  Forwarding via:
    (i/f 0) 240.161.6.0
  Admin Distance: DIRECTLY_CONNECTED
  Counter Id: counter0
  Class Id: None
)";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
