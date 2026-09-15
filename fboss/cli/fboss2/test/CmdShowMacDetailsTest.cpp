// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/mac/CmdShowMacDetails.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowMacDetailsTestFixture : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowMacDetailsTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowMacDetailsTraits::description().empty());
  EXPECT_FALSE(CmdShowMacDetails::sampleModel().l2Entries()->empty());
}

} // namespace facebook::fboss
