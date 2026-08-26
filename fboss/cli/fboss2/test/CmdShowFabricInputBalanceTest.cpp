// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/fabric/inputbalance/CmdShowFabricInputBalance.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowFabricInputBalanceTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowFabricInputBalanceTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowFabricInputBalanceTraits::description().empty());
  EXPECT_FALSE(
      CmdShowFabricInputBalance::sampleModel().inputBalanceEntry()->empty());
}

} // namespace facebook::fboss
