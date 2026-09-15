// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/interface/counters/fec/uncorrectable/CmdShowInterfaceCountersFecUncorrectable.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowInterfaceCountersFecUncorrectableTestFixture
    : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowInterfaceCountersFecUncorrectableTestFixture, wikiDocHooks) {
  EXPECT_FALSE(
      CmdShowInterfaceCountersFecUncorrectableTraits::description().empty());
  EXPECT_FALSE(
      CmdShowInterfaceCountersFecUncorrectable::sampleModel()
          .uncorrectableFrames()
          ->empty());
}

} // namespace facebook::fboss
