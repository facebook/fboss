// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteSummary.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::vector<UnicastRoute> createUnicastRouteEntries() {
  std::vector<UnicastRoute> routeEntries;
  UnicastRoute routeEntry1, routeEntry2, routeEntry3, routeEntry4;

  // routeEntry1
  folly::IPAddressV6 ip1("2401:db00::");
  network::thrift::BinaryAddress binaryAddr1 =
      facebook::network::toBinaryAddress(ip1);

  IpPrefix ipPrefix1;
  ipPrefix1.ip() = binaryAddr1;
  ipPrefix1.prefixLength() = 32;
  routeEntry1.dest() = ipPrefix1;

  // routeEntry2
  folly::IPAddressV4 ip2("176.161.6.0");
  network::thrift::BinaryAddress binaryAddr2 =
      facebook::network::toBinaryAddress(ip2);

  IpPrefix ipPrefix2;
  ipPrefix2.ip() = binaryAddr2;
  ipPrefix2.prefixLength() = 32;
  routeEntry2.dest() = ipPrefix2;

  // routeEntry3
  folly::IPAddressV6 ip3("2401:db00:e32f:8fc::2");
  network::thrift::BinaryAddress binaryAddr3 =
      facebook::network::toBinaryAddress(ip3);

  IpPrefix ipPrefix3;
  ipPrefix3.ip() = binaryAddr3;
  ipPrefix3.prefixLength() = 80;
  routeEntry3.dest() = ipPrefix3;

  // routeEntry4
  folly::IPAddressV4 ip4("240.161.6.0");
  network::thrift::BinaryAddress binaryAddr4 =
      facebook::network::toBinaryAddress(ip4);

  IpPrefix ipPrefix4;
  ipPrefix4.ip() = binaryAddr4;
  ipPrefix4.prefixLength() = 32;
  routeEntry4.dest() = ipPrefix4;

  routeEntries.emplace_back(routeEntry1);
  routeEntries.emplace_back(routeEntry2);
  routeEntries.emplace_back(routeEntry3);
  routeEntries.emplace_back(routeEntry4);
  return routeEntries;
}

cli::ShowRouteSummaryModel createRouteSummaryModel() {
  cli::ShowRouteSummaryModel model;

  model.numV4Routes() = 2;
  model.numV6Small() = 1;
  model.numV6Big() = 1;
  model.numV6() = 2;
  model.hwEntriesUsed() = 8;
  return model;
}

class CmdShowRouteSummaryTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<facebook::fboss::UnicastRoute> routeEntries;
  cli::ShowRouteSummaryModel normalizedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    routeEntries = createUnicastRouteEntries();
    normalizedModel = createRouteSummaryModel();
  }
};

TEST_F(CmdShowRouteSummaryTestFixture, createModel) {
  auto cmd = CmdShowRouteSummary();
  auto model = cmd.createModel(routeEntries);

  EXPECT_THRIFT_EQ(normalizedModel, model);
}

TEST_F(CmdShowRouteSummaryTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getRouteTable(_))
      .WillOnce(Invoke([&](auto& entries) { entries = routeEntries; }));

  auto cmd = CmdShowRouteSummary();
  auto model = cmd.queryClient(localhost());

  EXPECT_THRIFT_EQ(normalizedModel, model);
}

TEST_F(CmdShowRouteSummaryTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowRouteSummary().printOutput(normalizedModel, ss);

  std::string output = ss.str();
  std::string expectOutput =
      "Route Table Summary:\n\n"
      "         2 v4 routes\n"
      "         1 v6 routes (/64 or smaller)\n"
      "         1 v6 routes (bigger than /64)\n"
      "         2 v6 routes (total)\n"
      "         8 approximate hw entries used\n\n";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
