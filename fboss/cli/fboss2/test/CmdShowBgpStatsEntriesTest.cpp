// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/commands/show/bgp/stats/CmdShowBgpStatsEntries.h"

using namespace ::testing;
using facebook::neteng::fboss::bgp::thrift::TEntryStats;
namespace facebook::fboss {

const int kTotalUcastRoutes = 30;
const int kTotalRibPaths = 20;
const int kTotalAdjRibs = 15;
const int kTotalOriginatedRoutes = 2;
const int kTotalShadowRibEntries = 10;
const int kTotalNetlinkInterfaces = 32;

class CmdShowBgpStatsEntriesTestFixture : public CmdHandlerTestBase {
 public:
  TEntryStats stats_;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    stats_ = getStats();
  }

  TEntryStats getStats() {
    TEntryStats queriedStats;

    queriedStats.total_ucast_routes() = kTotalUcastRoutes;
    queriedStats.total_rib_paths() = kTotalRibPaths;
    queriedStats.total_adj_ribs() = kTotalAdjRibs;
    queriedStats.total_originated_routes() = kTotalOriginatedRoutes;
    queriedStats.total_shadow_rib_entries() = kTotalShadowRibEntries;
    queriedStats.total_netlink_wrapper_interfaces() = kTotalNetlinkInterfaces;

    return queriedStats;
  }
};

TEST_F(CmdShowBgpStatsEntriesTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getEntryStats(_))
      .WillOnce(Invoke([&](auto& entries) { entries = stats_; }));

  auto results = CmdShowBgpStatsEntries().queryClient(localhost());
  EXPECT_EQ(kTotalUcastRoutes, results.total_ucast_routes());
  EXPECT_EQ(kTotalRibPaths, results.total_rib_paths());
  EXPECT_EQ(kTotalAdjRibs, results.total_adj_ribs());
  EXPECT_EQ(kTotalOriginatedRoutes, results.total_originated_routes());
  EXPECT_EQ(kTotalShadowRibEntries, results.total_shadow_rib_entries());
  EXPECT_EQ(
      kTotalNetlinkInterfaces, results.total_netlink_wrapper_interfaces());
}

TEST_F(CmdShowBgpStatsEntriesTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowBgpStatsEntries().printOutput(stats_, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP entry statistics:\n"
      " Total number of unicast routes: 30\n"
      " Total number of rib paths: 20\n"
      " Total number of adjribs: 15\n"
      " Total number of originated routes: 2\n"
      " Total number of shadow rib entries: 10\n"
      " Total number of tracked netlink wrapper interfaces: 32\n";

  EXPECT_EQ(expectedOutput, output);
}
} // namespace facebook::fboss
