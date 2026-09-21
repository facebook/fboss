// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/dsf/subscription/CmdShowDsfSubscription.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowDsfSubscriptionTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowDsfSubscriptionTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowDsfSubscriptionTraits::description().empty());
  EXPECT_FALSE(CmdShowDsfSubscription::sampleModel().subscriptions()->empty());
}

} // namespace facebook::fboss
