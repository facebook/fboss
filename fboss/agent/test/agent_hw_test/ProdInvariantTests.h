#include <vector>
#include "fboss/agent/Main.h"
#include "fboss/agent/test/agent_hw_test/ProdAgentTests.h"

class HwSwitch;

namespace facebook::fboss {

class ProdInvariantTest : public ProdAgentTests {
 protected:
  virtual void SetUp() override;
  virtual void setupConfigFlag() override;
  virtual cfg::SwitchConfig initialConfig();
  void verifyAcl();
  void verifyCopp();
  void verifySafeDiagCommands();
  void verifyLoadBalancing();
  void verifyDscpToQueueMapping();
  void verifyQueuePerHostMapping(bool dscpMarkingTest);
  std::vector<PortDescriptor> ecmpPorts_{};
  bool checkBaseConfigPortsEmpty();
  cfg::SwitchConfig getConfigFromFlag();

 private:
  std::vector<PortID> getEcmpPortIds();
  void sendTraffic();
  PortID getDownlinkPort();
  void setupAgentTestEcmp(const std::vector<PortDescriptor>& ecmpPorts);
  std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports);
};

int ProdInvariantTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);
} // namespace facebook::fboss
