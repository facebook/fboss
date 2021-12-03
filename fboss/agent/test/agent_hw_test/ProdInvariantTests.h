#include "fboss/agent/Main.h"
#include "fboss/agent/test/agent_hw_test/ProdAgentTests.h"

class HwSwitch;

namespace facebook::fboss {

class ProdInvariantTest : public ProdAgentTests {
 protected:
  void SetUp() override;
  void setupConfigFlag() override;
  cfg::SwitchConfig initialConfig();
  void verifyCopp();

 private:
  void sendTraffic();
  void setupAgentTestEcmp(const std::vector<PortDescriptor>& ecmpPorts);
};

int ProdInvariantTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);
} // namespace facebook::fboss
