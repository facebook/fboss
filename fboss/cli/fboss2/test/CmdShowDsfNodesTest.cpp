// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/dsfnodes/CmdShowDsfNodes.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowDsfNodesTestFixture : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowDsfNodesTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowDsfNodesTraits::description().empty());
  EXPECT_FALSE(CmdShowDsfNodes::sampleModel().dsfNodes()->empty());
}

} // namespace facebook::fboss
