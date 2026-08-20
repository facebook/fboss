// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/transceiver/loopback/CmdShowTransceiverLoopback.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowTransceiverLoopbackTestFixture : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowTransceiverLoopbackTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowTransceiverLoopbackTraits::description().empty());
  EXPECT_FALSE(CmdShowTransceiverLoopback::sampleModel().empty());
}

} // namespace facebook::fboss
