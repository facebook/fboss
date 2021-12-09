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
  void verifyLoadBalancing();
  void verifyDscpToQueueMapping();

 private:
  std::vector<PortID> getEcmpPortIds();
  void sendTraffic();
  PortID getDownlinkPort();
  void setupAgentTestEcmp(const std::vector<PortDescriptor>& ecmpPorts);
  std::vector<PortDescriptor> ecmpPorts_{};
  std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports);
};

int ProdInvariantTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);
} // namespace facebook::fboss
