/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include "fboss/cli/fboss2/commands/show/bgp/stats/CmdShowBgpStatsAttrs.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

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

// An older bgpd predating the deduplicator fields sends them unset. The
// existing block must still render and the new one must be omitted entirely,
// rather than throwing on a missing optional.
TEST_F(CmdShowBgpStatsAttrsTestFixture, printOutputOmitsDedupBlockWhenUnset) {
  std::stringstream ss;
  CmdShowBgpStatsAttrs().printOutput(stats_, ss);

  EXPECT_THAT(ss.str(), HasSubstr("BGP attribute statistics:"));
  EXPECT_THAT(ss.str(), Not(HasSubstr("Deduplicator sizes")));
}

TEST_F(CmdShowBgpStatsAttrsTestFixture, printOutputRendersDedupSizes) {
  auto stats = stats_;
  stats.dedup_bgp_path() = 800000;
  stats.dedup_bgp_attributes() = 300001;
  stats.dedup_as_path() = 101;
  stats.dedup_communities() = 200;
  stats.dedup_cluster_list() = 0;
  stats.dedup_ext_communities() = 100;

  std::stringstream ss;
  CmdShowBgpStatsAttrs().printOutput(stats, ss);
  const std::string output = ss.str();

  // The pre-existing block is untouched -- this is an extension, not a
  // replacement.
  EXPECT_THAT(output, HasSubstr(" Total number of attributes: 1"));
  EXPECT_THAT(output, HasSubstr(" Average as path length: 1.06"));

  EXPECT_THAT(output, HasSubstr("Deduplicator sizes"));
  EXPECT_THAT(output, HasSubstr("bgp_attributes"));
  EXPECT_THAT(output, HasSubstr("300001"));

  /*
   * The L2 collection is published to fb303 as "...deduplicated_attributes.
   * total", a name that reads as a grand total when it is the bundle count.
   * The CLI must not reproduce it.
   */
  EXPECT_THAT(output, Not(HasSubstr("total)")));
  EXPECT_THAT(output, HasSubstr("101"));
  EXPECT_THAT(output, HasSubstr("800000"));
  EXPECT_THAT(output, HasSubstr("bgp_path"));

  /*
   * The level column is what stops "total" being read as a grand total, and
   * the trailing note is what stops L2 being read as the sum of L3. Both are
   * load-bearing for interpreting the numbers, so both are asserted.
   */
  EXPECT_THAT(output, HasSubstr("Level"));
  EXPECT_THAT(output, HasSubstr("L1"));
  EXPECT_THAT(output, HasSubstr("L2"));
  EXPECT_THAT(output, HasSubstr("L3"));
  EXPECT_THAT(output, HasSubstr("NOT the sum of the level below"));
}

// A zero-valued collection must render as 0, not be mistaken for unset and
// suppressed -- cluster_list is legitimately 0 without route reflection.
TEST_F(CmdShowBgpStatsAttrsTestFixture, printOutputRendersZeroCollection) {
  auto stats = stats_;
  stats.dedup_bgp_path() = 1;
  stats.dedup_cluster_list() = 0;

  std::stringstream ss;
  CmdShowBgpStatsAttrs().printOutput(stats, ss);

  EXPECT_THAT(ss.str(), HasSubstr("cluster_list"));
  EXPECT_THAT(ss.str(), HasSubstr("Deduplicator sizes"));
}
} // namespace facebook::fboss
