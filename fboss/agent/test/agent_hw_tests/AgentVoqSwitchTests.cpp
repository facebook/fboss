// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestFabricUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/SplitAgentTest.h"

DECLARE_bool(enable_stats_update_thread);

namespace facebook::fboss {
class AgentVoqSwitchTest : public SplitAgentTest {
 public:
  cfg::SwitchConfig initialConfig(
      SwSwitch* swSwitch,
      const std::vector<PortID>& ports) const override {
    // Disable sw stats update thread
    FLAGS_enable_stats_update_thread = false;
    // Before m-mpu agent test, use first Asic for initialization.
    auto switchIds = swSwitch->getHwAsicTable()->getSwitchIDs();
    CHECK_GE(switchIds.size(), 1);
    auto asic = swSwitch->getHwAsicTable()->getHwAsic(*switchIds.cbegin());
    auto config = utility::onePortPerInterfaceConfig(
        swSwitch->getPlatformMapping(),
        asic,
        ports,
        asic->desiredLoopbackModes(),
        true /*interfaceHasSubnet*/);
    const auto& cpuStreamTypes =
        asic->getQueueStreamTypes(cfg::PortType::CPU_PORT);
    for (const auto& cpuStreamType : cpuStreamTypes) {
      if (asic->getDefaultNumPortQueues(
              cpuStreamType, cfg::PortType::CPU_PORT)) {
        // cpu queues supported
        addCpuTrafficPolicy(config, asic);
        utility::addCpuQueueConfig(config, asic);
        break;
      }
    }
    return config;
  }

  void SetUp() override {
    SplitAgentTest::SetUp();
    ASSERT_TRUE(
        std::any_of(getAsics().begin(), getAsics().end(), [](auto& iter) {
          return iter.second->getSwitchType() == cfg::SwitchType::VOQ;
        }));
  }

 private:
  void addCpuTrafficPolicy(cfg::SwitchConfig& cfg, const HwAsic* asic) const {
    cfg::CPUTrafficPolicyConfig cpuConfig;
    std::vector<cfg::PacketRxReasonToQueue> rxReasonToQueues;
    std::vector<std::pair<cfg::PacketRxReason, uint16_t>>
        rxReasonToQueueMappings = {
            std::pair(
                cfg::PacketRxReason::BGP, utility::getCoppHighPriQueueId(asic)),
            std::pair(
                cfg::PacketRxReason::BGPV6,
                utility::getCoppHighPriQueueId(asic)),
            std::pair(
                cfg::PacketRxReason::CPU_IS_NHOP, utility::kCoppMidPriQueueId),
        };
    for (auto rxEntry : rxReasonToQueueMappings) {
      auto rxReasonToQueue = cfg::PacketRxReasonToQueue();
      rxReasonToQueue.rxReason() = rxEntry.first;
      rxReasonToQueue.queueId() = rxEntry.second;
      rxReasonToQueues.push_back(rxReasonToQueue);
    }
    cpuConfig.rxReasonToQueueOrderedList() = rxReasonToQueues;
    cfg.cpuTrafficPolicy() = cpuConfig;
  }

 protected:
  void addRemoveNeighbor(
      PortDescriptor port,
      bool add,
      std::optional<int64_t> encapIdx = std::nullopt) {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState());
    if (add) {
      getAgentEnsemble()->applyNewState(ecmpHelper.resolveNextHops(
          getProgrammedState(), {port}, false /*use link local*/, encapIdx));
    } else {
      getAgentEnsemble()->applyNewState(
          ecmpHelper.unresolveNextHops(getProgrammedState(), {port}));
    }
  }
};

class AgentVoqSwitchWithFabricPortsTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      SwSwitch* swSwitch,
      const std::vector<PortID>& ports) const override {
    // Disable sw stats update thread
    FLAGS_enable_stats_update_thread = false;
    // Before m-mpu agent test, use first Asic for initialization.
    auto switchIds = swSwitch->getHwAsicTable()->getSwitchIDs();
    CHECK_GE(switchIds.size(), 1);
    auto asic = swSwitch->getHwAsicTable()->getHwAsic(*switchIds.cbegin());
    auto config = utility::onePortPerInterfaceConfig(
        swSwitch->getPlatformMapping(),
        asic,
        ports,
        asic->desiredLoopbackModes(),
        true, /*interfaceHasSubnet*/
        true, /*setInterfaceMac*/
        utility::kBaseVlanId,
        true /*enable fabric ports*/);
    populatePortExpectedNeighbors(ports, config);
    return config;
  }

 private:
  bool hideFabricPorts() const override {
    return false;
  }
};

TEST_F(AgentVoqSwitchTest, addRemoveNeighbor) {
  auto setup = [this]() {
    const PortDescriptor kPort(getAgentEnsemble()->masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[0]);
    // Add neighbor
    addRemoveNeighbor(kPort, true);
    // Remove neighbor
    addRemoveNeighbor(kPort, false);
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchWithFabricPortsTest, init) {
  auto setup = []() {};

  auto verify = [this]() {
    auto state = getProgrammedState();
    for (auto& portMap : std::as_const(*state->getPorts())) {
      for (auto& port : std::as_const(*portMap.second)) {
        if (port.second->isEnabled()) {
          EXPECT_EQ(
              port.second->getLoopbackMode(),
              // TODO: Handle multiple Asics
              getAsics().cbegin()->second->getDesiredLoopbackMode(
                  port.second->getPortType()));
        }
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
