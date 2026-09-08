// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/fabric/topology/CmdShowFabricTopology.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

namespace facebook::fboss {

class CmdShowFabricTopologyTestFixture : public CmdHandlerTestBase {};

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowFabricTopologyTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowFabricTopologyTraits::description().empty());
  EXPECT_FALSE(
      CmdShowFabricTopology::sampleModel().virtualDeviceTopology()->empty());
}

} // namespace facebook::fboss
