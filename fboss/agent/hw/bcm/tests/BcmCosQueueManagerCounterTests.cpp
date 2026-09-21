/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManager.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

namespace facebook::fboss {

class BcmQueueManagerCounterTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }
};

} // namespace facebook::fboss

TEST_F(BcmQueueManagerCounterTest, SetupPortQueueCounters) {
  auto cfg = initialConfig();
  applyNewConfig(cfg);

  auto bcmPort = getHwSwitch()->getPortTable()->getBcmPort(PortID(1));
  auto queueManager = bcmPort->getQueueManager();
  queueManager->setupQueueCounters();

  const auto& queueCounters = queueManager->getQueueCounters();
  EXPECT_EQ(queueCounters.size(), queueManager->getQueueCounterTypes().size());
  for (const auto& type : queueManager->getQueueCounterTypes()) {
    auto curCountersItr = queueCounters.find(type);
    EXPECT_TRUE(curCountersItr != queueCounters.end());
    if (type.isScopeQueues()) {
      if (!isSupported(HwAsic::Feature::L3_QOS)) {
        EXPECT_EQ(curCountersItr->second.queues.size(), 0);
      } else {
        EXPECT_EQ(
            curCountersItr->second.queues.size(),
            queueManager->getNumQueues(type.streamType));
      }
    }
    if (type.isScopeAggregated()) {
      EXPECT_TRUE(curCountersItr->second.aggregated != nullptr);
    }
  }
}

TEST_F(BcmQueueManagerCounterTest, SetupCPUQueueCounters) {
  auto cfg = initialConfig();
  applyNewConfig(cfg);

  auto queueManager = getHwSwitch()->getControlPlane()->getQueueManager();
  const auto& queueCounters = queueManager->getQueueCounters();
  EXPECT_EQ(queueCounters.size(), queueManager->getQueueCounterTypes().size());
  for (const auto& type : queueManager->getQueueCounterTypes()) {
    auto curCountersItr = queueCounters.find(type);
    EXPECT_TRUE(curCountersItr != queueCounters.end());
    if (type.isScopeQueues()) {
      // We should always expect # of cpu queue counters > 0
      EXPECT_TRUE(curCountersItr->second.queues.size() > 0);
      EXPECT_EQ(
          curCountersItr->second.queues.size(),
          queueManager->getNumQueues(type.streamType));
    }
    // for now we should not have aggregated support for cpu
    EXPECT_EQ(curCountersItr->second.aggregated, nullptr);
  }
}
