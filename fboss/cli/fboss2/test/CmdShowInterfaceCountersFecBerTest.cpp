// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/interface/counters/fec/ber/CmdShowInterfaceCountersFecBer.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowInterfaceCountersFecBerTestFixture : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowInterfaceCountersFecBerTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowInterfaceCountersFecBerTraits::description().empty());
  EXPECT_FALSE(CmdShowInterfaceCountersFecBer::sampleModel().fecBer()->empty());
}

} // namespace facebook::fboss
