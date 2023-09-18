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
    : public HwLoadBalancerTest<utility::HwIpV6RoCEEcmpDataPlaneTestUtil> {
  std::unique_ptr<utility::HwIpV6RoCEEcmpDataPlaneTestUtil> getECMPHelper()
      override {
    return std::make_unique<utility::HwIpV6RoCEEcmpDataPlaneTestUtil>(
        getHwSwitchEnsemble(), RouterID(0));
  }

 private:
  cfg::SwitchConfig initialConfig() const override {
    auto hwSwitch = getHwSwitch();
    auto cfg = utility::onePortPerInterfaceConfig(
        hwSwitch, masterLogicalPortIds(), getAsic()->desiredLoopbackModes());
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

  void SetUp() override {
    FLAGS_flowletSwitchingEnable = true;
    HwLoadBalancerTest::SetUp();
  }
};

RUN_HW_LOAD_BALANCER_TEST_FOR_DLB(
    HwLoadBalancerTestV6Flowlet,
    Ecmp,
    Full,
    FrontPanel)

} // namespace facebook::fboss
