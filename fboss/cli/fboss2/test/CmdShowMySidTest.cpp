// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/mysid/CmdShowMySid.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowMySidTestFixture : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowMySidTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowMySidTraits::description().empty());
  EXPECT_FALSE(CmdShowMySid::sampleModel().mySidEntries()->empty());
}

} // namespace facebook::fboss
