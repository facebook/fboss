// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

namespace facebook::fboss {

class AgentOverflowTestBase : public AgentHwTest {
 protected:
  void SetUp() override;
  cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) const override;
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override;
  void startPacketTxRxVerify();
  void stopPacketTxRxVerify();

  PortID getDownlinkPort();
  std::vector<PortID> getUplinkPorts();

  void verifyInvariants();

 private:
  std::atomic<bool> packetRxVerifyRunning_{false};
  std::unique_ptr<std::thread> packetRxVerifyThread_;
};
} // namespace facebook::fboss
