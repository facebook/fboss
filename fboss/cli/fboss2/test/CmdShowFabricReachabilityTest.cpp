// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/fabric/reachability/CmdShowFabricReachability.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowFabricReachabilityTestFixture : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowFabricReachabilityTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowFabricReachabilityTraits::description().empty());
  EXPECT_FALSE(
      CmdShowFabricReachability::sampleModel().reachabilityEntries()->empty());
}

} // namespace facebook::fboss
