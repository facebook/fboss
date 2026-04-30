// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#ifndef IS_OSS
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include <string_view>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include "fboss/cli/fboss2/commands/show/bgp/stats/CmdShowBgpStatsPolicy.h"
#include "neteng/routing/policy/if/gen-cpp2/policy_thrift_types.h"

using namespace ::testing;
using facebook::neteng::routing::policy::thrift::TPolicyStatementStats;
using facebook::neteng::routing::policy::thrift::TPolicyStats;
using facebook::neteng::routing::policy::thrift::TPolicyTermStats;
namespace facebook::fboss {

const std::string_view kStatementName = "RSW-TEST";
const int kStatementPrefixHitCount = 300;
const int kStatementAvgTime = 12;
const int kStatementMaxTime = 200;
const int kStatementNumRuns = 300;
const std::string_view kTermName = "term-test";
const std::string_view kTermDescription = "backup anycast path";
const int kTermPrefixHitCount = 200;

class CmdShowBgpStatsPolicyTestFixture : public CmdHandlerTestBase {
 public:
  TPolicyStats stats_;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    stats_ = getStats();
  }

  TPolicyStats getStats() {
    TPolicyStatementStats statement_stats;
    statement_stats.name() = kStatementName;
    statement_stats.prefix_hit_count() = kStatementPrefixHitCount;
    statement_stats.avg_time() = kStatementAvgTime;
    statement_stats.max_time() = kStatementMaxTime;
    statement_stats.num_of_runs() = kStatementNumRuns;

    TPolicyTermStats term_stats;
    term_stats.name() = kTermName;
    term_stats.description() = kTermDescription;
    term_stats.prefix_hit_count() = kTermPrefixHitCount;

    statement_stats.term_stats() = {term_stats};

    TPolicyStats queriedSats;
    queriedSats.policy_statement_stats() = {statement_stats};

    return queriedSats;
  }
};

TEST_F(CmdShowBgpStatsPolicyTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getPolicyStats(_))
      .WillOnce(Invoke([&](auto& entries) { entries = stats_; }));

  auto results = CmdShowBgpStatsPolicy().queryClient(localhost());

  const auto& statements = results.policy_statement_stats().value();
  ASSERT_EQ(statements.size(), 1);

  const auto& st_stats = statements[0];
  EXPECT_EQ(st_stats.get_name(), kStatementName);
  EXPECT_EQ(st_stats.get_prefix_hit_count(), kStatementPrefixHitCount);
  EXPECT_EQ(st_stats.get_avg_time(), kStatementAvgTime);
  EXPECT_EQ(st_stats.get_max_time(), kStatementMaxTime);
  EXPECT_EQ(st_stats.get_num_of_runs(), kStatementNumRuns);

  const auto& term_stats = st_stats.term_stats().value();
  ASSERT_EQ(term_stats.size(), 1);

  const auto& tr_stats = term_stats[0];
  EXPECT_EQ(tr_stats.get_name(), kTermName);
  EXPECT_EQ(tr_stats.get_description(), kTermDescription);
  EXPECT_EQ(tr_stats.get_prefix_hit_count(), kTermPrefixHitCount);
}

TEST_F(CmdShowBgpStatsPolicyTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowBgpStatsPolicy().printOutput(stats_, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      "BGP policy statistics:"
      "\n Policy Name: RSW-TEST"
      "\n\t Number of executions: 300"
      "\n\t Number of hits: 300 prefixes"
      "\n\t Average time of execution: 12us"
      "\n\t Maximum time of execution: 200us"
      "\n\t Term  Description                                        Hits/Misses"
      "\n\t 1     backup anycast path                                 200/100   "
      "\n\t 2     Default deny (Implicit)                             100/0\n";

  EXPECT_EQ(expectedOutput, output);
}
} // namespace facebook::fboss
#endif // IS_OSS
