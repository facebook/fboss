// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/agent/CmdShowAgentFirmware.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowAgentFirmwareTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowAgentFirmwareTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowAgentFirmwareTraits::description().empty());
  EXPECT_FALSE(CmdShowAgentFirmware::sampleModel().firmwareEntries()->empty());
}

} // namespace facebook::fboss
