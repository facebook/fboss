// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/NetworkAITestUtils.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

enum class NeighborOp { ADD, DEL };

class AgentVoqSwitchTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    {
      // Increase the query timeout to be 5sec
      FLAGS_hwswitch_query_timeout = 5000;
      auto config = utility::onePortPerInterfaceConfig(
          ensemble.getSw(),
          ensemble.masterLogicalPortIds(),
          true /*interfaceHasSubnet*/);
      utility::addNetworkAIQosMaps(config, ensemble.getL3Asics());
      utility::setDefaultCpuTrafficPolicyConfig(
          config, ensemble.getL3Asics(), ensemble.isSai());
      utility::addCpuQueueConfig(
          config, ensemble.getL3Asics(), ensemble.isSai());
      return config;
    }
  }

  void SetUp() override {
    AgentHwTest::SetUp();
    if (!IsSkipped()) {
      ASSERT_TRUE(
          std::any_of(getAsics().begin(), getAsics().end(), [](auto& iter) {
            return iter.second->getSwitchType() == cfg::SwitchType::VOQ;
          }));
    }
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::VOQ};
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
      cfg::Scope portScope) {
    auto switchId =
        scopeResolver().scope(getProgrammedState(), port).switchId();
    const auto& dsfNode =
        getProgrammedState()->getDsfNodes()->getNodeIf(switchId);
    auto sysPortOffset = portScope == cfg::Scope::GLOBAL
        ? dsfNode->getGlobalSystemPortOffset()
        : dsfNode->getLocalSystemPortOffset();
    CHECK(sysPortOffset.has_value());
    return SystemPortID(port.intID() + *sysPortOffset);
  }

  std::string kDscpAclName() const {
    return "dscp_acl";
  }
  std::string kDscpAclCounterName() const {
    return "dscp_acl_counter";
  }

  void addDscpAclWithCounter();
  void addRemoveNeighbor(PortDescriptor port, NeighborOp operation);
  void setForceTrafficOverFabric(bool force);
  void setupForDramErrorTestFromDiagShell(const SwitchID& switchId);

  std::vector<PortDescriptor> getInterfacePortSysPortDesc() {
    auto ports = getProgrammedState()->getPorts()->getAllNodes();
    std::vector<PortDescriptor> portDescs;
    std::for_each(
        ports->begin(),
        ports->end(),
        [this, &portDescs](const auto& idAndPort) {
          const auto port = idAndPort.second;
          if (port->getPortType() == cfg::PortType::INTERFACE_PORT ||
              port->getPortType() == cfg::PortType::HYPER_PORT) {
            portDescs.push_back(PortDescriptor(getSystemPortID(
                PortDescriptor(port->getID()), cfg::Scope::GLOBAL)));
          }
        });
    return portDescs;
  }

  // Resolve and return list of local nhops (only NIF ports)
  std::vector<PortDescriptor> resolveLocalNhops(
      utility::EcmpSetupTargetedPorts6& ecmpHelper) {
    std::vector<PortDescriptor> portDescs = getInterfacePortSysPortDesc();

    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      for (const auto& portDesc : portDescs) {
        out = ecmpHelper.resolveNextHops(out, {portDesc});
      }
      return out;
    });
    return portDescs;
  }

  std::string getSdkMajorVersion(const SwitchID& switchId);
};
} // namespace facebook::fboss
