// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class AgentVoqSwitchTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) const override;

  void SetUp() override {
    AgentHwTest::SetUp();
    if (!IsSkipped()) {
      ASSERT_TRUE(
          std::any_of(getAsics().begin(), getAsics().end(), [](auto& iter) {
            return iter.second->getSwitchType() == cfg::SwitchType::VOQ;
          }));
    }
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::VOQ};
  }

 protected:
  void
  rxPacketToCpuHelper(uint16_t l4SrcPort, uint16_t l4DstPort, uint8_t queueId);
  // API to send a local service discovery packet which is an IPv6
  // multicast paket with UDP payload. This packet with a destination
  // MAC as the unicast NIF port MAC helps recreate CS00012323788.
  void sendLocalServiceDiscoveryMulticastPacket(
      const PortID outPort,
      const int numPackets);
  int sendPacket(
      const folly::IPAddress& dstIp,
      std::optional<PortID> frontPanelPort,
      std::optional<std::vector<uint8_t>> payload =
          std::optional<std::vector<uint8_t>>(),
      int dscp = 0x24);
  SystemPortID getSystemPortID(
      const PortDescriptor& port,
      cfg::Scope portScope);
  std::string kDscpAclName() const {
    return "dscp_acl";
  }
  std::string kDscpAclCounterName() const {
    return "dscp_acl_counter";
  }

  void addDscpAclWithCounter();
  void addRemoveNeighbor(PortDescriptor port, bool add);
  void setForceTrafficOverFabric(bool force);
  std::vector<PortDescriptor> getInterfacePortSysPortDesc();
  // Resolve and return list of local nhops (only NIF ports)
  std::vector<PortDescriptor> resolveLocalNhops(
      utility::EcmpSetupTargetedPorts6& ecmpHelper);
};
} // namespace facebook::fboss
