/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using std::make_shared;

TEST(QcmConfigTest, applyConfig) {
  WeightMap map;

  map.emplace(10, 1); // default
  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  // make any random change, so that new state can be created
  // ensure that qcm is not configured
  config.qosPolicies_ref()->resize(1);
  auto& policy = config.qosPolicies_ref()[0];
  *policy.name_ref() = "qosPolicy";
  auto state0 = publishAndApplyConfig(state, &config, platform.get());
  EXPECT_EQ(state0->getQcmCfg(), nullptr);

  cfg::QcmConfig qcmCfg;
  *qcmCfg.numFlowsClear_ref() = 22;
  *qcmCfg.collectorDstIp_ref() = "10.10.10.10/32";
  *qcmCfg.collectorSrcIp_ref() = "11.11.11.11/24";
  qcmCfg.port2QosQueueIds_ref()[1] = {0, 1, 2};
  qcmCfg.port2QosQueueIds_ref()[2] = {3, 4, 5};

  config.qcmConfig_ref() = qcmCfg;
  auto state1 = publishAndApplyConfig(state0, &config, platform.get());
  EXPECT_NE(nullptr, state1);
  auto qcmConfig1 = state1->getQcmCfg();
  EXPECT_TRUE(qcmConfig1);
  EXPECT_FALSE(qcmConfig1->isPublished());
  EXPECT_EQ(qcmConfig1->getNumFlowsClear(), 22);
  EXPECT_EQ(qcmConfig1->getFlowWeightMap(), map);
  // default should kick in
  EXPECT_EQ(
      qcmConfig1->getNumFlowSamplesPerView(),
      cfg::switch_config_constants::DEFAULT_QCM_FLOWS_PER_VIEW());
  EXPECT_EQ(
      qcmConfig1->getFlowLimit(),
      cfg::switch_config_constants::DEFAULT_QCM_FLOW_LIMIT());
  EXPECT_EQ(
      qcmConfig1->getCollectorDstIp(),
      folly::CIDRNetwork(folly::IPAddress("10.10.10.10"), 32));
  EXPECT_EQ(
      qcmConfig1->getCollectorSrcIp(),
      folly::CIDRNetwork(folly::IPAddress("11.11.11.0"), 24));
  EXPECT_FALSE(qcmConfig1->getMonitorQcmCfgPortsOnly());
  auto portList = qcmConfig1->getMonitorQcmPortList();
  EXPECT_EQ(portList.size(), 0);
  // verify port2QosQueueIds field
  int initPortId = 1;
  int initQosQueueId = 0;
  auto port2QosQueueIds = qcmConfig1->getPort2QosQueueIdMap();
  for (const auto& perPortQosQueueIds : port2QosQueueIds) {
    EXPECT_EQ(perPortQosQueueIds.first, initPortId++);
    for (const auto& qosQueueId : perPortQosQueueIds.second) {
      EXPECT_EQ(qosQueueId, initQosQueueId++);
    }
  }

  // re-program the map
  map.emplace(1, 9);
  qcmCfg.flowWeights_ref()[cfg::BurstMonitorWeight::FLOW_SUM_RX_BYTES] = 9;
  *qcmCfg.numFlowSamplesPerView_ref() = 11;
  *qcmCfg.flowLimit_ref() = 13;
  *qcmCfg.exportThreshold_ref() = 20;
  *qcmCfg.agingIntervalInMsecs_ref() = 21;
  *qcmCfg.monitorQcmPortList_ref() = {102, 103, 104, 105};
  *qcmCfg.port2QosQueueIds_ref() = {};
  qcmCfg.port2QosQueueIds_ref()[3] = {6, 7};
  int collectorDscp = 20;
  qcmCfg.collectorDscp_ref() = collectorDscp;
  int ppsToQcm = 1000;
  qcmCfg.ppsToQcm_ref() = ppsToQcm;
  qcmCfg.monitorQcmCfgPortsOnly_ref() = true;

  config.qcmConfig_ref() = qcmCfg;
  auto state2 = publishAndApplyConfig(state1, &config, platform.get());
  EXPECT_NE(nullptr, state2);
  auto qcmConfig2 = state2->getQcmCfg();
  EXPECT_FALSE(qcmConfig2->isPublished());
  EXPECT_EQ(qcmConfig2->getNumFlowsClear(), 22);
  EXPECT_EQ(qcmConfig2->getNumFlowSamplesPerView(), 11);
  EXPECT_EQ(qcmConfig2->getFlowWeightMap(), map);
  EXPECT_EQ(qcmConfig2->getFlowLimit(), 13);
  EXPECT_EQ(qcmConfig2->getAgingInterval(), 21);
  EXPECT_EQ(qcmConfig2->getCollectorDscp(), 20);
  EXPECT_EQ(qcmConfig2->getPpsToQcm(), 1000);
  EXPECT_TRUE(qcmConfig2->getMonitorQcmCfgPortsOnly());
  portList = qcmConfig2->getMonitorQcmPortList();
  EXPECT_EQ(4, portList.size());

  port2QosQueueIds = qcmConfig2->getPort2QosQueueIdMap();
  for (const auto& perPortQosQueueIds : port2QosQueueIds) {
    EXPECT_EQ(perPortQosQueueIds.first, initPortId++);
    for (const auto& qosQueueId : perPortQosQueueIds.second) {
      EXPECT_EQ(qosQueueId, initQosQueueId++);
    }
  }

  // remove the cfg
  config.qcmConfig_ref().reset();
  auto state3 = publishAndApplyConfig(state2, &config, platform.get());
  EXPECT_NE(nullptr, state3);
  EXPECT_FALSE(state3->getQcmCfg());
}

TEST(QcmConfigTest, ToFromJSON) {
  std::string jsonStr = R"(
        {
          "agingIntervalInMsecs": 10,
          "numFlowSamplesPerView": 10,
          "flowLimit": 10,
          "numFlowsClear": 10,
          "scanIntervalInUsecs": 10,
          "exportThreshold": 10,
          "flowWeights": {
            "0": 1,
            "1": 2,
            "2": 3
          },
          "collectorDstIp": "11::01/128",
          "collectorSrcPort": 1000,
          "collectorDscp": 20,
          "ppsToQcm": 1000,
          "collectorSrcIp" : "10::01/128",
          "monitorQcmPortList": [
            112,
            113,
            114
          ],
          "port2QosQueueIds": {
            "10": [
              0,
              1,
              2,
              3
            ],
            "11": [
              4,
              5,
              6,
              7
            ]
           },
           "monitorQcmCfgPortsOnly": true
        }
  )";

  auto qcmCfg = QcmCfg::fromFollyDynamic(folly::parseJson(jsonStr));
  EXPECT_EQ(10, qcmCfg->getAgingInterval());
  EXPECT_EQ(10, qcmCfg->getNumFlowSamplesPerView());
  EXPECT_EQ(10, qcmCfg->getFlowLimit());
  EXPECT_EQ(10, qcmCfg->getNumFlowsClear());
  EXPECT_EQ(10, qcmCfg->getScanIntervalInUsecs());
  EXPECT_EQ(10, qcmCfg->getExportThreshold());
  EXPECT_TRUE(qcmCfg->getMonitorQcmCfgPortsOnly());

  int initPortId = 10;
  int initQosQueueId = 0;
  const auto& port2QosQueueIds = qcmCfg->getPort2QosQueueIdMap();
  for (const auto& perPortQosQueueIds : port2QosQueueIds) {
    EXPECT_EQ(perPortQosQueueIds.first, initPortId++);
    for (const auto& qosQueueId : perPortQosQueueIds.second) {
      EXPECT_EQ(qosQueueId, initQosQueueId++);
    }
  }

  int weightKeyValues = 0;
  int weightDataValues = 1;
  for (const auto& weight : qcmCfg->getFlowWeightMap()) {
    EXPECT_EQ(weight.first, weightKeyValues++);
    EXPECT_EQ(weight.second, weightDataValues++);
  }
  EXPECT_EQ(
      folly::CIDRNetwork(folly::IPAddress("11::01"), 128),
      qcmCfg->getCollectorDstIp());
  EXPECT_EQ(1000, qcmCfg->getCollectorSrcPort());
  EXPECT_EQ(20, qcmCfg->getCollectorDscp());
  EXPECT_EQ(1000, qcmCfg->getPpsToQcm());
  EXPECT_EQ(
      folly::CIDRNetwork(folly::IPAddress("10::01"), 128),
      qcmCfg->getCollectorSrcIp());
  auto portList = qcmCfg->getMonitorQcmPortList();
  EXPECT_EQ(portList.size(), 3);
}

// Intent of this test is to enable QCM, modify an
// unrelated switch setting such as l2 learn mode and
// ensure that QCM setings are preserved
TEST(QcmConfigTest, verifyQcmWithSwitchSettingsChange) {
  WeightMap map;

  auto platform = createMockPlatform();
  auto state = make_shared<SwitchState>();

  // apply QCM config
  cfg::SwitchConfig config;
  cfg::QcmConfig qcmCfg;
  qcmCfg.numFlowsClear_ref() = 22;
  qcmCfg.collectorDstIp_ref() = "10.10.10.10/32";
  qcmCfg.collectorSrcIp_ref() = "11.11.11.11/24";
  config.qcmConfig_ref() = qcmCfg;
  auto state0 = publishAndApplyConfig(state, &config, platform.get());

  // verify state is valid
  EXPECT_NE(nullptr, state0);

  // Toggle switch setting for l2 learn
  config.switchSettings_ref()->l2LearningMode_ref() =
      cfg::L2LearningMode::SOFTWARE;
  auto state1 = publishAndApplyConfig(state0, &config, platform.get());
  EXPECT_NE(nullptr, state1);

  // verify that QCM configs are preserved
  auto qcmConfig1 = state1->getQcmCfg();
  EXPECT_NE(nullptr, qcmConfig1);
  EXPECT_EQ(qcmConfig1->getNumFlowsClear(), 22);
}
