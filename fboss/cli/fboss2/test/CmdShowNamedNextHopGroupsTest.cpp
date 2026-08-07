// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddress.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/commands/show/nexthopgroups/CmdShowNextHopGroups.h"
#include "fboss/cli/fboss2/commands/show/nexthopgroups/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

namespace {
NextHopThrift makeNextHop(const std::string& ip) {
  NextHopThrift nh;
  nh.address() = facebook::network::toBinaryAddress(folly::IPAddress(ip));
  return nh;
}

// Two distinct named groups that share the SAME next-hop set. On hardware they
// dedup to a single NextHopSetId; the name-keyed getNamedNextHopGroups API
// still returns both, one entry per name.
std::vector<NextHopGroup> createSameSetNamedGroups() {
  NextHopGroup groupA;
  groupA.name() = "nhgA";
  groupA.isProgrammed() = true;
  groupA.nexthops() = {makeNextHop("2401:db00::1")};

  NextHopGroup groupB;
  groupB.name() = "nhgB";
  groupB.isProgrammed() = true;
  groupB.nexthops() = {makeNextHop("2401:db00::1")};

  return {groupA, groupB};
}
} // namespace

class CmdShowNamedNextHopGroupsTestFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
  }
};

// Two named groups sharing one next-hop set must both appear in the listing.
// This is the regression guard for the collapse bug: the command must query the
// name-keyed getNamedNextHopGroups API rather than the set-id-keyed
// getNextHopGroups API (which returns only one entry per shared NextHopSetId).
TEST_F(CmdShowNamedNextHopGroupsTestFixture, queryClientReturnsBothSharedSet) {
  setupMockedAgentServer();
  auto groups = createSameSetNamedGroups();
  EXPECT_CALL(getMockAgent(), getNamedNextHopGroups(_, _))
      .WillOnce(Invoke([&](auto& result, auto /*names*/) { result = groups; }));

  auto cmd = CmdShowNamedNextHopGroups();
  auto model = cmd.queryClient(localhost());

  const auto& entries = model.nextHopGroups().value();
  ASSERT_EQ(entries.size(), 2);

  std::vector<std::string> names{
      entries[0].name().value(), entries[1].name().value()};
  std::sort(names.begin(), names.end());
  const std::vector<std::string> expected{"nhgA", "nhgB"};
  EXPECT_EQ(names, expected);
}

} // namespace facebook::fboss
