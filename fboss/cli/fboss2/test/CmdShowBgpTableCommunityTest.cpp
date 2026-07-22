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
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/reflection/testing.h> // NOLINT(misc-include-cleaner)
#include <vector>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include <folly/json/json.h>

#include "fboss/cli/fboss2/commands/show/bgp/CanonicalRibResolver.h"
#include "fboss/cli/fboss2/commands/show/bgp/table/CmdShowBgpTableCommunity.h"
#include "fboss/cli/fboss2/test/CmdBgpTestUtils.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#ifndef IS_OSS
// Avoid EXPECT_THRIFT_EQ clash with <thrift/lib/cpp2/reflection/testing.h>
#undef EXPECT_THRIFT_EQ
#include "nettools/common/TestUtils.h"
#endif

using namespace ::testing;
using namespace facebook::neteng::fboss::bgp::thrift;
namespace facebook::fboss {
const std::string groupNameA = "groupNameA";
const int asnA = 123;
const int valueA = 4567;
const std::string groupNameB = "groupNameB";
const int asnB = 891;
const int valueB = 1112;
const std::string communityString = "123:4567";

class CmdShowBgpTableCommunityTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<TRibEntry> entries_;
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    entries_ = {
        buildEntryByCommunity(groupNameA, asnA, valueA),
        buildEntryByCommunity(groupNameB, asnB, valueB)};
  }
};

TEST_F(CmdShowBgpTableCommunityTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRunningConfig(_))
      .Times(::testing::AnyNumber())
      .WillRepeatedly([&](std::string& config) { config = "{}"; });

  auto canonical = buildCanonicalRibState();
  EXPECT_CALL(
      getMockBgp(), getRibEntriesForCommunityCanonical(_, TBgpAfi::AFI_IPV4, _))
      .WillOnce([&](TCanonicalRibState& state,
                    TBgpAfi,
                    std::unique_ptr<std::string>) { state = canonical; });
  EXPECT_CALL(
      getMockBgp(), getRibEntriesForCommunityCanonical(_, TBgpAfi::AFI_IPV6, _))
      .WillOnce([&](TCanonicalRibState& state,
                    TBgpAfi,
                    std::unique_ptr<std::string>) { state = canonical; });

  auto result =
      CmdShowBgpTableCommunity().queryClient(localhost(), {communityString});
  const auto perAfiExpected = resolveCanonicalRibState(canonical);
  std::vector<TRibEntry> expected;
  expected.insert(expected.end(), perAfiExpected.begin(), perAfiExpected.end());
  expected.insert(expected.end(), perAfiExpected.begin(), perAfiExpected.end());
  EXPECT_THRIFT_EQ_VECTOR(*result.tRibEntries(), expected);
}

TEST_F(CmdShowBgpTableCommunityTestFixture, filterEntriesByCommunities) {
  auto results = CmdShowBgpTableCommunity().filterEntriesByCommunities(
      entries_, communityString);

  ASSERT_EQ(results.size(), 1);
  const auto& entry = results.at(0);

  EXPECT_EQ(entry.get_best_group(), groupNameA);
  const auto& paths = entry.paths().value();
  ASSERT_EQ(paths.size(), 1);
  const auto& communities =
      apache::thrift::get_pointer(paths.at(groupNameA).at(0).communities());
  ASSERT_EQ(communities->size(), 1);
  EXPECT_EQ(communities->at(0).get_asn(), asnA);
  EXPECT_EQ(communities->at(0).get_value(), valueA);
}
} // namespace facebook::fboss
