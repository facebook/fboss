// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/commands/show/bgp/stats/CmdShowBgpStatsAttrs.h"

using namespace ::testing;
using facebook::neteng::fboss::bgp::thrift::TAttributeStats;
namespace facebook::fboss {

const int kTotalNumberOfAttributes = 1;
const int KTotalUniqueAttributes = 2;
const float kAvgAttrRefCount = 1.3578;
const float kAvgCommunityListLen = 0.0;
const float kAvgExtCommunityListLen = 3.4567;
const float kAvgASPathLen = 1.064;
const float kAvgClusterListLen = 3.1415;
const float kAvgTopologyInfoLen = 1.618;

class CmdShowBgpStatsAttrsTestFixture : public CmdHandlerTestBase {
 public:
  TAttributeStats stats_;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    stats_ = getStats();
  }

  TAttributeStats getStats() {
    TAttributeStats queriedStats;

    queriedStats.total_num_of_attributes() = kTotalNumberOfAttributes;
    queriedStats.total_unique_attributes() = KTotalUniqueAttributes;
    queriedStats.avg_attribute_refcount() = kAvgAttrRefCount;
    queriedStats.avg_community_list_len() = kAvgCommunityListLen;
    queriedStats.avg_extcommunity_list_len() = kAvgExtCommunityListLen;
    queriedStats.avg_as_path_len() = kAvgASPathLen;
    queriedStats.avg_cluster_list_len() = kAvgClusterListLen;
    queriedStats.avg_topology_info_len() = kAvgTopologyInfoLen;

    return queriedStats;
  }
};

TEST_F(CmdShowBgpStatsAttrsTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getAttributeStats(_))
      .WillOnce(Invoke([&](auto& entries) { entries = stats_; }));

  auto results = CmdShowBgpStatsAttrs().queryClient(localhost());
  EXPECT_EQ(kTotalNumberOfAttributes, results.get_total_num_of_attributes());
  EXPECT_EQ(KTotalUniqueAttributes, results.get_total_unique_attributes());
  EXPECT_EQ(kAvgAttrRefCount, results.get_avg_attribute_refcount());
  EXPECT_EQ(kAvgCommunityListLen, results.get_avg_community_list_len());
  EXPECT_EQ(kAvgExtCommunityListLen, results.get_avg_extcommunity_list_len());
  EXPECT_EQ(kAvgASPathLen, results.get_avg_as_path_len());
  EXPECT_EQ(kAvgClusterListLen, results.avg_cluster_list_len());
  EXPECT_EQ(kAvgTopologyInfoLen, results.avg_topology_info_len());
}

TEST_F(CmdShowBgpStatsAttrsTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowBgpStatsAttrs().printOutput(stats_, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "BGP attribute statistics:\n"
      " Total number of attributes: 1\n"
      " Total number of unique attributes: 2\n"
      " Average attribute reference count: 1.36\n"
      " Average community list length: 0.00\n"
      " Average extended community list length: 3.46\n"
      " Average as path length: 1.06\n"
      " Average cluster list length: 3.14\n"
      " Average topology info length: 1.62\n";

  EXPECT_EQ(expectedOutput, output);
}
} // namespace facebook::fboss
