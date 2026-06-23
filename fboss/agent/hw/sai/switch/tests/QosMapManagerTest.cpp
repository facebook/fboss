/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiQosMapManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/QosPolicy.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

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
      const SaiQosMapHandle* saiQosMapHandle,
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
    auto switchSettings =
        utility::getFirstNodeIf(newState->getSwitchSettings());
    if (switchSettings) {
      auto newSwitchSettings = switchSettings->modify(&newState);
      newSwitchSettings->setDefaultDataPlaneQosPolicy(policy);
    } else {
      auto newSwitchSettings = std::make_shared<SwitchSettings>();
      newSwitchSettings->setDefaultDataPlaneQosPolicy(policy);
      addSwitchSettingsToState(newState, newSwitchSettings);
    }
    return newState;
  }
};

TEST_F(QosMapManagerTest, addQosMap) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qp = makeQosPolicy("default", testQosPolicy);
  saiManagerTable->qosMapManager().addQosMap(qp, true);
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
  saiManagerTable->qosMapManager().addQosMap(qp, true);
  auto saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  EXPECT_TRUE(saiQosMapHandle);
  saiManagerTable->qosMapManager().removeQosMap(qp, true);
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
  saiManagerTable->qosMapManager().addQosMap(qp, true);
  auto saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  EXPECT_TRUE(saiQosMapHandle);

  TestQosPolicy testQosPolicy2{{11, 2, 4}, {43, 0, 4}, {1, 1, 1}};
  auto qp2 = makeQosPolicy("default", testQosPolicy2);
  saiManagerTable->qosMapManager().changeQosMap(qp, qp2, true);
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
  auto qosPolicies = std::make_shared<MultiSwitchQosPolicyMap>();
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qosPolicy = makeQosPolicy("qos", testQosPolicy);
  qosPolicies->addNode(qosPolicy, scopeResolver().scope(qosPolicy));
  switchState->resetQosPolicies(qosPolicies);
  state::PortFields portFields;
  portFields.portId() = PortID(1);
  portFields.portName() = "eth1/1/1";
  auto port = std::make_shared<Port>(std::move(portFields));
  port->setQosPolicy("qos");
  switchState->getPorts()->addNode(port, scopeResolver().scope(port));
  EXPECT_TRUE(saiPlatform->getHwSwitch()->isValidStateUpdate(
      StateDelta(std::make_shared<SwitchState>(), switchState)));
}

// By default the PFC priority -> priority group map is not programmed into
// SAI even when configured, since the map type is unsupported on some
// platforms. Programming is opt-in via a feature flag.
TEST_F(QosMapManagerTest, pfcPriorityToPgQosMapNotProgrammedByDefault) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qp = makeQosPolicy("default", testQosPolicy);
  qp->setPfcPriorityToPgIdMap({{0, 5}, {7, 3}});

  saiManagerTable->qosMapManager().addQosMap(qp, true);
  auto saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  EXPECT_TRUE(saiQosMapHandle);
  EXPECT_FALSE(saiQosMapHandle->pfcPriorityToPgMap);
}

// With the feature flag on, the PFC priority -> priority group map is
// programmed into SAI with the configured entries.
TEST_F(QosMapManagerTest, pfcPriorityToPgQosMapProgrammedWhenEnabled) {
  gflags::FlagSaver flagSaver;
  FLAGS_enable_pfc_priority_to_pg_map = true;
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qp = makeQosPolicy("default", testQosPolicy);
  const std::map<int16_t, int16_t> pfcPriorityToPg{{0, 5}, {7, 3}};
  qp->setPfcPriorityToPgIdMap(pfcPriorityToPg);

  saiManagerTable->qosMapManager().addQosMap(qp, true);
  auto saiQosMapHandle = saiManagerTable->qosMapManager().getQosMap();
  EXPECT_TRUE(saiQosMapHandle);
  EXPECT_TRUE(saiQosMapHandle->pfcPriorityToPgMap);

  auto& qosMapApi = SaiApiTable::getInstance()->qosMapApi();
  EXPECT_EQ(
      qosMapApi.getAttribute(
          saiQosMapHandle->pfcPriorityToPgMap->adapterKey(),
          SaiQosMapTraits::Attributes::Type{}),
      SAI_QOS_MAP_TYPE_PFC_PRIORITY_TO_PRIORITY_GROUP);
  auto mapToValueList = qosMapApi.getAttribute(
      saiQosMapHandle->pfcPriorityToPgMap->adapterKey(),
      SaiQosMapTraits::Attributes::MapToValueList{});
  std::map<int16_t, int16_t> gotPfcPriorityToPg;
  for (const auto& entry : mapToValueList) {
    gotPfcPriorityToPg.emplace(entry.key.prio, entry.value.pg);
  }
  EXPECT_EQ(gotPfcPriorityToPg, pfcPriorityToPg);
}

TEST_F(QosMapManagerTest, getPfcPriorityToQueueId) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qp = makeQosPolicy("default", testQosPolicy);
  const std::map<int16_t, int16_t> pfcPriorityToQueue{{3, 7}, {4, 2}};
  qp->setPfcPriorityToQueueIdMap(pfcPriorityToQueue);

  saiManagerTable->qosMapManager().addQosMap(qp, true);
  EXPECT_EQ(
      saiManagerTable->qosMapManager().getPfcPriorityToQueueId(),
      pfcPriorityToQueue);
}

TEST_F(QosMapManagerTest, getPfcPriorityToQueueIdEmptyWhenAbsent) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto qp = makeQosPolicy("default", testQosPolicy);
  // No pfcPriorityToQueueId on the policy => empty.
  saiManagerTable->qosMapManager().addQosMap(qp, true);
  EXPECT_TRUE(
      saiManagerTable->qosMapManager().getPfcPriorityToQueueId().empty());
}

TEST_F(QosMapManagerTest, changAddsPortQos) {
  TestQosPolicy testQosPolicy{{10, 0, 2}, {42, 1, 4}};
  auto oldState = makeSwitchState(makeQosPolicy("qos", testQosPolicy));
  state::PortFields portFields;
  portFields.portId() = PortID(1);
  portFields.portName() = "eth1/1/1";
  auto port = std::make_shared<Port>(std::move(portFields));
  port->setQosPolicy("qos");
  auto ports = std::make_shared<MultiSwitchPortMap>();
  ports->addNode(port, scopeResolver().scope(port));
  oldState->resetPorts(ports);
  oldState->publish();
  EXPECT_TRUE(saiPlatform->getHwSwitch()->isValidStateUpdate(
      StateDelta(std::make_shared<SwitchState>(), oldState)));
  auto newState = oldState->clone();
  auto newPort = port->modify(&newState);
  auto qosPolicies = std::make_shared<MultiSwitchQosPolicyMap>();
  auto qosPolicy = makeQosPolicy("qos", testQosPolicy);
  qosPolicies->addNode(qosPolicy, scopeResolver().scope(qosPolicy));
  newState->resetQosPolicies(qosPolicies);
  auto switchSettings = utility::getFirstNodeIf(newState->getSwitchSettings());
  auto newSwitchSettings = switchSettings->modify(&newState);
  newSwitchSettings->setDefaultDataPlaneQosPolicy(nullptr);
  newPort->setQosPolicy("qos");
  EXPECT_TRUE(saiPlatform->getHwSwitch()->isValidStateUpdate(
      StateDelta(oldState, newState)));
}
