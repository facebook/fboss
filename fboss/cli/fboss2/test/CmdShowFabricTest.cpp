// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/fabric/CmdShowFabric.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowFabricTestFixture : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowFabricTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowFabricTraits::description().empty());
  EXPECT_FALSE(CmdShowFabric::sampleModel().fabricEntries()->empty());
}

} // namespace facebook::fboss
