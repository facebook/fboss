// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <folly/IPAddressV6.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowRouteTestFixture : public CmdHandlerTestBase {};

TEST(CmdShowRouteTest, AnnotatesBackupNextHops) {
  UnicastRoute route;
  route.dest() = IpPrefix();
  route.dest()->ip() =
      network::toBinaryAddress(folly::IPAddressV6("2401:db00::"));
  route.dest()->prefixLength() = 64;

  NextHopThrift primary;
  primary.address() = network::toBinaryAddress(folly::IPAddressV6("1::1"));

  NextHopThrift backup;
  backup.address() = network::toBinaryAddress(folly::IPAddressV6("2::1"));
  backup.role() = NextHopRole::BACKUP;

  route.nextHops() = {primary, backup};
  std::vector<UnicastRoute> routes{route};

  auto cmd = CmdShowRoute();
  auto model = cmd.createModel(routes);
  std::stringstream output;
  cmd.printOutput(model, output);

  EXPECT_EQ(
      output.str(),
      "Network Address: 2401:db00::/64\n"
      "\tvia 1::1\n"
      "\tvia 2::1 (BACKUP)\n");
}

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowRouteTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowRouteTraits::description().empty());
  EXPECT_FALSE(CmdShowRoute::sampleModel().routeEntries()->empty());
}

} // namespace facebook::fboss
