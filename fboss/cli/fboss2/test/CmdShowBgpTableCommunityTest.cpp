// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include <folly/json/json.h>

#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/commands/show/bgp/table/CmdShowBgpTableCommunity.h"
#include "fboss/cli/fboss2/test/CmdBgpTestUtils.h"

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

// Since we are not testing filtering and output is already
// being tested in other table command unit tests, we just want
// to make sure the right thrift call is being made.
TEST_F(CmdShowBgpTableCommunityTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRibEntriesForCommunity(_, _, _));
  EXPECT_CALL(getMockBgp(), getRunningConfig(_))
      .WillOnce(Invoke([&](std::string& config) {
        // clang-format off
        folly::dynamic value = folly::dynamic::object
          ("communities",
          folly::dynamic::array(
          folly::dynamic::object("name", "FABRIC_POD_RSW_LOOP")
          ("description", "rsw loopback")
          ("communities", folly::dynamic::array("65527:12705"))
          )
        )
        ("localprefs",
        folly::dynamic::array(
          folly::dynamic::object("localpref", 20)
          ("name", "LOCALPREF_CTRL_BACKUP")
          ("description", "low-priority supplementary/backup routes from bgp controller"))
        );
        // clang-format on
        config = folly::toPrettyJson(value);
      }));
  auto result =
      CmdShowBgpTableCommunity().queryClient(localhost(), {communityString});
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
