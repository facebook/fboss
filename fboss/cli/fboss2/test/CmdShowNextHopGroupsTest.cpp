// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/nexthopgroups/CmdShowNextHopGroups.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowNextHopGroupsTestFixture : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowNextHopGroupsTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowNextHopGroupsTraits::description().empty());
  EXPECT_FALSE(CmdShowNextHopGroups::sampleModel().nextHopGroups()->empty());
}

} // namespace facebook::fboss
