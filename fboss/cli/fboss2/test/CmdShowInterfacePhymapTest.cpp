// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>

#include "fboss/cli/fboss2/commands/show/interface/phymap/CmdShowInterfacePhymap.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdShowInterfacePhymapTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowInterfacePhymapTestFixture, noXphyPlatform) {
  setupMockedAgentServer();
  EXPECT_CALL(getQsfpService(), getMacsecCapablePorts(_))
      .WillOnce(Invoke([](auto& ports) { ports.clear(); }));

  auto model = CmdShowInterfacePhymap().queryClient(localhost(), {});
  EXPECT_FALSE(model.portsPhyMap().value().macsecPortPhyMap().has_value());

  std::stringstream output;
  CmdShowInterfacePhymap().printOutput(model, output);
  EXPECT_EQ("No Phy port map for this platform\n", output.str());
}

} // namespace facebook::fboss
