/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
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
    setupStage = SetupStage::BLANK;
    ManagerTestBase::SetUp();
  }
  void validateQosPolicy(
      SaiQosMapHandle* saiQosMapHandle,
      TestQosPolicy testQosPolicy) {
    EXPECT_TRUE(saiQosMapHandle);
    EXPECT_TRUE(saiQosMapHandle->dscpQosMap);
    EXPECT_TRUE(saiQosMapHandle->tcQosMap);
    auto& qosMapApi = SaiApiTable::getInstance()->qosMapApi();
    auto dscpMapToValueList = qosMapApi.getAttribute(
        saiQosMapHandle->dscpQosMap->adapterKey(),
        SaiQosMapTraits::Attributes::MapToValueList{});
    auto tcMapToValueList = qosMapApi.getAttribute(
        saiQosMapHandle->tcQosMap->adapterKey(),
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
    auto newState = in->clone();
    newState->setDefaultDataPlaneQosPolicy(policy);
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
  saiPlatform->getHwSwitch()->stateChanged(
      StateDelta(std::make_shared<SwitchState>(), newState));
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
