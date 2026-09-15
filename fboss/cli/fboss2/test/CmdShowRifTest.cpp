// (c) Meta Platforms, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/rif/CmdShowRif.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowRifTestFixture : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowRifTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowRifTraits::description().empty());
  EXPECT_FALSE(CmdShowRif::sampleModel().rifs()->empty());
}

} // namespace facebook::fboss
