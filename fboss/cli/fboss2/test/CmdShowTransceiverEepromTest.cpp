// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/transceiver/eeprom/CmdShowTransceiverEeprom.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowTransceiverEepromTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowTransceiverEepromTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowTransceiverEepromTraits::description().empty());
  EXPECT_FALSE(CmdShowTransceiverEeprom::sampleModel().empty());
}

} // namespace facebook::fboss
