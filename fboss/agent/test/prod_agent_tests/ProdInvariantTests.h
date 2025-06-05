#pragma once
#include <vector>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/Main.h"
#include "fboss/agent/SetupThrift.h"
#include "fboss/agent/test/prod_agent_tests/ProdAgentTests.h"

class HwSwitch;

namespace facebook::fboss {

class ProdInvariantTest : public ProdAgentTests {
 protected:
  void sendTraffic(int numPackets);
  void setupAgentTestEcmp(const std::vector<PortDescriptor>& ecmpPorts);
  virtual void SetUp() override;
  virtual void setupConfigFlag();
  cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) override;
  virtual cfg::SwitchConfig getConfigFromFlag();
  void verifyAcl();
  void verifyCopp();
  void verifySafeDiagCommands();
  void verifyLoadBalancing(int numPackets = 10000);
  void verifyDscpToQueueMapping();
  void verifyQueuePerHostMapping(bool dscpMarkingTest);
  std::vector<PortDescriptor> ecmpPorts_{};
  bool checkBaseConfigPortsEmpty();
  void verifyThriftHandler();
  void verifySwSwitchHandler();
  void verifyHwSwitchHandler();
  void set_mmu_lossless(bool mmu_lossless) {
    mmuLosslessMode_ = mmu_lossless;
  }
  bool is_mmu_lossless_mode() {
    return mmuLosslessMode_;
  }
  std::vector<PortID> getAllPlatformPorts(
      const std::map<int32_t, cfg::PlatformPortEntry>& platformPorts);
  void printDiagCmd(const std::string& cmd);

 protected:
  std::optional<bool> useProdConfig_ = std::nullopt;
  PortID getDownlinkPort();
  std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports);
  std::vector<PortID> getEcmpPortIds();
  void setCmdLineFlagOverrides() const override {
    ProdAgentTests::setCmdLineFlagOverrides();
    FLAGS_prod_invariant_config_test = true;
  }

 private:
  bool mmuLosslessMode_ = false;
};

int ProdInvariantTestMain(int argc, char** argv, PlatformInitFn initPlatformFn);
} // namespace facebook::fboss
