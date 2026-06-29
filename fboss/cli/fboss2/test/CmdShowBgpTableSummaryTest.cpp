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
#include <cstdint>
#include <cstdlib>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "fboss/cli/fboss2/commands/show/bgp/table/CmdShowBgpTableSummary.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

using namespace ::testing;
using namespace facebook::neteng::fboss::bgp::thrift;
using facebook::neteng::fboss::bgp_attr::TBgpAfi;

namespace facebook::fboss {

namespace {
TRibSummary makeSummary(
    TBgpAfi afi,
    int64_t totalPrefixes,
    std::map<int16_t, int64_t> lengthCounts) {
  TRibSummary summary;
  summary.afi() = afi;
  summary.total_prefixes() = totalPrefixes;
  summary.prefix_length_counts() = std::move(lengthCounts);
  return summary;
}
} // namespace

class CmdShowBgpTableSummaryTestFixture : public CmdHandlerTestBase {
 public:
  TRibSummary v4_;
  TRibSummary v6_;
  std::optional<std::string> savedLcAll_;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    // utils::Table (tabulate) constructs std::locale("") from the environment;
    // pin a valid locale so the test is hermetic where LANG/LC_* are unset.
    // Saved and restored in TearDown so it does not leak into sibling tests.
    if (const char* lcAll = std::getenv("LC_ALL")) {
      savedLcAll_ = lcAll;
    }
    setenv("LC_ALL", "C", 1);
    v4_ = makeSummary(TBgpAfi::AFI_IPV4, 5, {{24, 3}, {32, 2}});
    v6_ = makeSummary(TBgpAfi::AFI_IPV6, 1, {{64, 1}});
  }

  void TearDown() override {
    if (savedLcAll_.has_value()) {
      setenv("LC_ALL", savedLcAll_->c_str(), 1);
    } else {
      unsetenv("LC_ALL");
    }
    CmdHandlerTestBase::TearDown();
  }
};

TEST_F(CmdShowBgpTableSummaryTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRibSummary(_, TBgpAfi::AFI_IPV4))
      .WillOnce([&](TRibSummary& summary, TBgpAfi) { summary = v4_; });
  EXPECT_CALL(getMockBgp(), getRibSummary(_, TBgpAfi::AFI_IPV6))
      .WillOnce([&](TRibSummary& summary, TBgpAfi) { summary = v6_; });

  auto result = CmdShowBgpTableSummary().queryClient(localhost());
  ASSERT_EQ(2, result.summaries()->size());
  EXPECT_EQ(TBgpAfi::AFI_IPV4, result.summaries()->at(0).afi());
  EXPECT_EQ(5, result.summaries()->at(0).total_prefixes());
  EXPECT_EQ(TBgpAfi::AFI_IPV6, result.summaries()->at(1).afi());
  EXPECT_EQ(1, result.summaries()->at(1).total_prefixes());
}

TEST_F(CmdShowBgpTableSummaryTestFixture, printOutput) {
  cli::ShowBgpTableSummaryModel model;
  model.summaries() = {v4_, v6_};

  std::stringstream ss;
  CmdShowBgpTableSummary().printOutput(model, ss);
  std::string output = ss.str();

  EXPECT_THAT(output, HasSubstr("Address Family: AFI_IPV4"));
  EXPECT_THAT(output, HasSubstr("Total Prefixes: 5"));
  EXPECT_THAT(output, HasSubstr("/24"));
  EXPECT_THAT(output, HasSubstr("/32"));
  EXPECT_THAT(output, HasSubstr("Address Family: AFI_IPV6"));
  EXPECT_THAT(output, HasSubstr("Total Prefixes: 1"));
  EXPECT_THAT(output, HasSubstr("/64"));
}

} // namespace facebook::fboss
