/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_hw_tests/AgentQosSchedulerTestBase.h"
#include "fboss/agent/test/utils/NetworkAITestUtils.h"

namespace facebook::fboss {
class AgentNetworkAIQosSchedulerTest : public AgentQosSchedulerTestBase {
 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    auto hwAsic = utility::checkSameAndGetAsic(ensemble.getL3Asics());
    auto streamType =
        *hwAsic->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT).begin();
    utility::addNetworkAIQueueConfig(
        &cfg, streamType, cfg::QueueScheduling::STRICT_PRIORITY, hwAsic);
    utility::addNetworkAIQosMaps(cfg, ensemble.getL3Asics());
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), cfg);
    return cfg;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::L3_QOS,
        production_features::ProductionFeature::NETWORKAI_QOS};
  }

  void verifyWRR();
  void verifySP(bool frontPanelTraffic = true);
  void verifyWRRAndSP(const std::vector<int>& queueIds, int trafficQueueId);
  void verifyWRRAndICP();
  void verifyWRRAndNC();
};

void AgentNetworkAIQosSchedulerTest::verifySP(bool frontPanelTraffic) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper6(getProgrammedState(), dstMac());

  auto setup = [=, this]() { _setup(ecmpHelper6); };

  auto verify = [=, this]() {
    EXPECT_TRUE(verifySPHelper(
        // SP queue with highest queueId
        // should starve other SP queues
        // altogether
        utility::getNetworkAIQueueId(utility::NetworkAIQueueType::NC),
        utility::kNetworkAISPQueueIds(),
        utility::kNetworkAIQueueToDscp(),
        frontPanelTraffic));
  };

  verifyAcrossWarmBoots(setup, verify);
}

void AgentNetworkAIQosSchedulerTest::verifyWRR() {
  auto setup = [=, this]() {
    auto newCfg{initialConfig(*getAgentEnsemble())};
    auto hwAsic =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
    auto streamType =
        *hwAsic->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT).begin();
    utility::addNetworkAIQueueConfig(
        &newCfg,
        streamType,
        cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN,
        hwAsic);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    EXPECT_TRUE(verifyWRRHelper(
        utility::getMaxWeightWRRQueue(utility::kNetworkAIWRRQueueToWeight()),
        utility::kNetworkAIWRRQueueToWeight(),
        utility::kNetworkAIWRRQueueIds(),
        utility::kNetworkAIQueueToDscp()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

void AgentNetworkAIQosSchedulerTest::verifyWRRAndSP(
    const std::vector<int>& queueIds,
    int trafficQueueId) {
  auto setup = [=, this]() {
    auto newCfg{initialConfig(*getAgentEnsemble())};
    auto hwAsic =
        utility::checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
    auto streamType =
        *hwAsic->getQueueStreamTypes(cfg::PortType::INTERFACE_PORT).begin();
    utility::addNetworkAIQueueConfig(
        &newCfg,
        streamType,
        cfg::QueueScheduling::STRICT_PRIORITY,
        hwAsic,
        false,
        true,
        {{utility::NetworkAIQueueType::RDMA,
          cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN}});
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    EXPECT_TRUE(verifySPHelper(
        trafficQueueId, queueIds, utility::kNetworkAIQueueToDscp()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

void AgentNetworkAIQosSchedulerTest::verifyWRRAndICP() {
  verifyWRRAndSP(
      utility::kNetworkAIWRRAndICPQueueIds(),
      utility::getNetworkAIQueueId(
          utility::NetworkAIQueueType::MONITORING)); // SP should starve WRR
                                                     // queues altogether
}

void AgentNetworkAIQosSchedulerTest::verifyWRRAndNC() {
  verifyWRRAndSP(
      utility::kNetworkAIWRRAndNCQueueIds(),
      utility::getNetworkAIQueueId(
          utility::NetworkAIQueueType::NC)); // SP should starve WRR
                                             // queues altogether
}

TEST_F(AgentNetworkAIQosSchedulerTest, VerifyWRR) {
  verifyWRR();
}

TEST_F(AgentNetworkAIQosSchedulerTest, VerifySP) {
  verifySP();
}

TEST_F(AgentNetworkAIQosSchedulerTest, VerifyWRRAndICP) {
  verifyWRRAndICP();
}

TEST_F(AgentNetworkAIQosSchedulerTest, VerifyWRRAndNC) {
  verifyWRRAndNC();
}

} // namespace facebook::fboss
