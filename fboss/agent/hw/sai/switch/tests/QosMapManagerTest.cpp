/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiQosMapManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/QosPolicy.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

/*
 * In these tests, we will assume 4 ports with one lane each, with IDs
 */

class QosMapManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT;
    ManagerTestBase::SetUp();
  }
  void validateQosPolicy(
      SaiQosMapHandle* saiQosMapHandle,
      TestQosPolicy testQosPolicy) {
    EXPECT_TRUE(saiQosMapHandle);
    EXPECT_TRUE(saiQosMapHandle->dscpToTcMap);
    EXPECT_TRUE(saiQosMapHandle->tcToQueueMap);
    auto& qosMapApi = SaiApiTable::getInstance()->qosMapApi();
    auto dscpMapToValueList = qosMapApi.getAttribute(
        saiQosMapHandle->dscpToTcMap->adapterKey(),
        SaiQosMapTraits::Attributes::MapToValueList{});
    auto tcMapToValueList = qosMapApi.getAttribute(
        saiQosMapHandle->tcToQueueMap->adapterKey(),
        SaiQosMapTraits::Attributes::MapToValueList{});
    EXPECT_EQ(dscpMapToValueList.size(), testQosPolicy.size());
    EXPECT_EQ(tcMapToValueList.size(), testQosPolicy.size());
    for (const auto& mapping : testQosPolicy) {
      auto [dscp, tc, q] = mapping;
      bool foundDscp = false;
      bool foundTc = false;
      for (const auto& entry : dscpMapToValueList) {
        if (entry.key.dscp == dscp) {
          EXPECT_EQ(entry.value.tc, tc);
          foundDscp = true;
        }
      }
      for (const auto& entry : tcMapToValueList) {
        if (entry.key.tc == tc) {
          EXPECT_EQ(entry.value.queue_index, q);
          foundTc = true;
        }
      }
      EXPECT_TRUE(foundDscp);
      EXPECT_TRUE(foundTc);
    }
  }
  std::shared_ptr<SwitchState> makeSwitchState(
      std::shared_ptr<QosPolicy> policy,
      std::shared_ptr<SwitchState> in = std::make_shared<SwitchState>()) {
    in->publish();
    auto newState = in->clone();
    auto switchSettings = newState->getSwitchSettings();
    switchSettings = switchSettings->clone();
    switchSettings->setDefaultDataPlaneQosPolicy(policy);
    newState->resetSwitchSettings(switchSettings);
    return newState;
  }
};

TEST_F(QosMapManagerTest, addQosMap) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qp = makeQosPolicy("default", testQosPolicy);
  saiManagerTable->qosMapManager().addQosMap(qp);
  auto saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  validateQosPolicy(saiQosMapHandle, testQosPolicy);
}

TEST_F(QosMapManagerTest, addQosMapDelta) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto newState = makeSwitchState(makeQosPolicy("default", testQosPolicy));
  applyNewState(newState);
  auto saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  validateQosPolicy(saiQosMapHandle, testQosPolicy);
}

TEST_F(QosMapManagerTest, removeQosMap) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qp = makeQosPolicy("default", testQosPolicy);
  saiManagerTable->qosMapManager().addQosMap(qp);
  auto saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  EXPECT_TRUE(saiQosMapHandle);
  saiManagerTable->qosMapManager().removeQosMap();
  saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  EXPECT_FALSE(saiQosMapHandle);
}

TEST_F(QosMapManagerTest, removeQosMapDelta) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto newState = makeSwitchState(makeQosPolicy("default", testQosPolicy));
  applyNewState(newState);
  auto saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  validateQosPolicy(saiQosMapHandle, testQosPolicy);
  auto newerState = makeSwitchState(nullptr, newState);
  applyNewState(newerState);
  saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  EXPECT_FALSE(saiQosMapHandle);
}

TEST_F(QosMapManagerTest, changeQosMap) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qp = makeQosPolicy("default", testQosPolicy);
  saiManagerTable->qosMapManager().addQosMap(qp);
  auto saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  EXPECT_TRUE(saiQosMapHandle);

  TestQosPolicy testQosPolicy2{{11, 2, 4}, {43, 0, 4}, {1, 1, 1}};
  auto qp2 = makeQosPolicy("default", testQosPolicy2);
  saiManagerTable->qosMapManager().changeQosMap(qp, qp2);
  saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  EXPECT_TRUE(saiQosMapHandle);
  validateQosPolicy(saiQosMapHandle, testQosPolicy2);
}

TEST_F(QosMapManagerTest, changeQosMapDelta) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto newState = makeSwitchState(makeQosPolicy("default", testQosPolicy));
  applyNewState(newState);
  auto saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  validateQosPolicy(saiQosMapHandle, testQosPolicy);

  TestQosPolicy testQosPolicy2{{11, 2, 4}, {43, 0, 4}, {1, 1, 1}};
  auto newerState =
      makeSwitchState(makeQosPolicy("default", testQosPolicy2), newState);
  applyNewState(newerState);
  saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  EXPECT_TRUE(saiQosMapHandle);
  validateQosPolicy(saiQosMapHandle, testQosPolicy2);
}

TEST_F(QosMapManagerTest, validQosPolicy) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qosPolicy = makeQosPolicy("default", testQosPolicy);
  auto newState = makeSwitchState(qosPolicy);
  EXPECT_TRUE(saiPlatform->getHwSwitch()->isValidStateUpdate(
      StateDelta(std::make_shared<SwitchState>(), newState)));
}

TEST_F(QosMapManagerTest, missingTCToQ) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qosPolicy = makeQosPolicy("default", testQosPolicy);
  qosPolicy->setTrafficClassToQueueIdMap(std::map<int16_t, int16_t>{});
  auto newState = makeSwitchState(qosPolicy);
  EXPECT_FALSE(saiPlatform->getHwSwitch()->isValidStateUpdate(
      StateDelta(std::make_shared<SwitchState>(), newState)));
}

TEST_F(QosMapManagerTest, missingDscpMap) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qosPolicy = makeQosPolicy("default", testQosPolicy);
  qosPolicy->setDscpMap(DscpMap{});
  auto newState = makeSwitchState(qosPolicy);
  EXPECT_FALSE(saiPlatform->getHwSwitch()->isValidStateUpdate(
      StateDelta(std::make_shared<SwitchState>(), newState)));
}

TEST_F(QosMapManagerTest, expMap) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qosPolicy = makeQosPolicy("default", testQosPolicy);
  ExpMap exp;
  exp.addFromEntry(TrafficClass{1}, EXP{2});
  exp.addToEntry(TrafficClass{2}, EXP{1});
  qosPolicy->setExpMap(exp);
  auto newState = makeSwitchState(qosPolicy);
  EXPECT_TRUE(saiPlatform->getHwSwitch()->isValidStateUpdate(
      StateDelta(std::make_shared<SwitchState>(), newState)));
}

TEST_F(QosMapManagerTest, addPortQos) {
  auto switchState = std::make_shared<SwitchState>();
  auto qosPolicies = std::make_shared<QosPolicyMap>();
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  qosPolicies->addNode(makeQosPolicy("qos", testQosPolicy));
  switchState->resetQosPolicies(qosPolicies);
  state::PortFields portFields;
  portFields.portId() = PortID(1);
  portFields.portName() = "eth1/1/1";
  auto port = std::make_shared<Port>(std::move(portFields));
  port->setQosPolicy("qos");
  switchState->getMultiSwitchPorts()->addNode(
      port, scopeResolver().scope(port));
  EXPECT_FALSE(saiPlatform->getHwSwitch()->isValidStateUpdate(
      StateDelta(std::make_shared<SwitchState>(), switchState)));
}

TEST_F(QosMapManagerTest, changAddsPortQos) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto oldState = makeSwitchState(makeQosPolicy("qos", testQosPolicy));
  state::PortFields portFields;
  portFields.portId() = PortID(1);
  portFields.portName() = "eth1/1/1";
  auto port = std::make_shared<Port>(std::move(portFields));
  port->setQosPolicy("qos");
  auto ports = std::make_shared<PortMap>();
  ports->addPort(port);
  oldState->resetPorts(ports);
  oldState->publish();
  EXPECT_TRUE(saiPlatform->getHwSwitch()->isValidStateUpdate(
      StateDelta(std::make_shared<SwitchState>(), oldState)));
  auto newState = oldState->clone();
  auto newPort = port->modify(&newState);
  auto qosPolicies = std::make_shared<QosPolicyMap>();
  qosPolicies->addNode(makeQosPolicy("qos", testQosPolicy));
  newState->resetQosPolicies(qosPolicies);
  auto switchSettings = newState->getSwitchSettings();
  switchSettings = switchSettings->clone();
  switchSettings->setDefaultDataPlaneQosPolicy(nullptr);
  newState->resetSwitchSettings(switchSettings);
  newPort->setQosPolicy("qos");
  EXPECT_FALSE(saiPlatform->getHwSwitch()->isValidStateUpdate(
      StateDelta(oldState, newState)));
}
