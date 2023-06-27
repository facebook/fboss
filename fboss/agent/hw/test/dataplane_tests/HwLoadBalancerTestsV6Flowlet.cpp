/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTests.h"

namespace {
const int kScalingFactor = 100;
const int kLoadWeight = 70;
const int kQueueWeight = 30;
} // namespace

namespace facebook::fboss {

class HwLoadBalancerTestV6Flowlet
    : public HwLoadBalancerTest<utility::HwIpV6EcmpDataPlaneTestUtil> {
  std::unique_ptr<utility::HwIpV6EcmpDataPlaneTestUtil> getECMPHelper()
      override {
    return std::make_unique<utility::HwIpV6EcmpDataPlaneTestUtil>(
        getHwSwitchEnsemble(), RouterID(0));
  }

 private:
  std::vector<PortID> getPorts() const {
    std::vector<PortID> ports = {
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        masterLogicalPortIds()[2],
        masterLogicalPortIds()[3],
        masterLogicalPortIds()[4],
        masterLogicalPortIds()[5],
        masterLogicalPortIds()[6],
        masterLogicalPortIds()[7],
        masterLogicalPortIds()[8],
    };

    return ports;
  }

  cfg::SwitchConfig initialConfig() const override {
    auto hwSwitch = getHwSwitch();
    auto ports = getPorts();
    auto cfg = utility::onePortPerInterfaceConfig(
        hwSwitch, std::move(ports), getAsic()->desiredLoopbackModes());
    cfg::FlowletSwitchingConfig flowletCfg =
        utility::getDefaultFlowletSwitchingConfig();
    cfg.flowletSwitchingConfig() = flowletCfg;

    std::map<std::string, cfg::PortFlowletConfig> portFlowletCfgMap;
    cfg::PortFlowletConfig portFlowletConfig;
    portFlowletConfig.scalingFactor() = kScalingFactor;
    portFlowletConfig.loadWeight() = kLoadWeight;
    portFlowletConfig.queueWeight() = kQueueWeight;
    portFlowletCfgMap.insert(std::make_pair("default", portFlowletConfig));
    cfg.portFlowletConfigs() = portFlowletCfgMap;

    auto allPorts = masterLogicalInterfacePortIds();
    std::vector<PortID> portIds(
        allPorts.begin(), allPorts.begin() + allPorts.size());
    for (auto portId : portIds) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->flowletConfigName() = "default";
    }

    return cfg;
  }

  void setPortState() {
    auto hwEnsemble = getHwSwitchEnsemble();
    auto newState = hwEnsemble->getProgrammedState()->clone();
    for (int i = 0; i < 8; i++) {
      auto newPort = newState->getPorts()
                         ->getNodeIf(masterLogicalPortIds()[i])
                         ->modify(&newState);
      newPort->setLoopbackMode(cfg::PortLoopbackMode::PHY);
      newPort->setAdminState(cfg::PortState::ENABLED);
    }
    hwEnsemble->applyNewState(newState, true);
  }

  void SetUp() override {
    FLAGS_flowletSwitchingEnable = true;
    HwLoadBalancerTest::SetUp();
    // For DLB ports need to be in PHY loopback mode since
    // BRCM does not support MAC loopback for 100G links due to phy lane issues
    setPortState();
  }
};

// DLB expects traffic not to be balanced on all egress ports
// Hence do negative test
RUN_HW_LOAD_BALANCER_NEGATIVE_TEST_FRONT_PANEL(
    HwLoadBalancerTestV6Flowlet,
    Ecmp,
    Full)

} // namespace facebook::fboss
