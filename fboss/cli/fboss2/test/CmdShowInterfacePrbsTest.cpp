// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/interface/prbs/capabilities/CmdShowInterfacePrbsCapabilities.h"
#include "fboss/cli/fboss2/commands/show/interface/prbs/state/CmdShowInterfacePrbsState.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowInterfacePrbsTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowInterfacePrbsTestFixture, wikiDocHooksCapabilities) {
  EXPECT_FALSE(CmdShowInterfacePrbsCapabilitiesTraits::description().empty());
  EXPECT_FALSE(
      CmdShowInterfacePrbsCapabilities::sampleModel()
          .interfaceEntries()
          ->empty());
}

TEST_F(CmdShowInterfacePrbsTestFixture, wikiDocHooksState) {
  EXPECT_FALSE(CmdShowInterfacePrbsStateTraits::description().empty());
  EXPECT_FALSE(
      CmdShowInterfacePrbsState::sampleModel().interfaceEntries()->empty());
}

} // namespace facebook::fboss
