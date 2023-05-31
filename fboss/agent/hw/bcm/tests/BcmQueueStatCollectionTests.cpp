/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

#include "fboss/agent/hw/test/HwTestStatUtils.h"

namespace facebook::fboss {

TEST_F(BcmTest, onlyExpectedQueueStatsSeen) {
  auto setup = [this] {
    applyNewConfig(utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        {{cfg::PortType::INTERFACE_PORT, cfg::PortLoopbackMode::MAC}}));
  };
  auto verify = [this] {
    for (auto i = 0; i < 10; ++i) {
      updateHwSwitchStats(getHwSwitch());
    }
    for (auto portId : masterLogicalPortIds()) {
      auto port = getHwSwitch()->getPortTable()->getBcmPort(portId);
      EXPECT_TRUE(port->isEnabled());
      auto hwStats = port->getPortStats();
      auto numUcastQueues =
          port->getQueueManager()->getNumQueues(cfg::StreamType::UNICAST);
      auto queueStatMaps = {
          *hwStats->queueOutBytes_(), *hwStats->queueOutDiscardBytes_()};
      // Only expected number of queue stats should be present
      EXPECT_TRUE(std::all_of(
          queueStatMaps.begin(),
          queueStatMaps.end(),
          [numUcastQueues, port](const auto& queueStatMap) {
            return numUcastQueues == queueStatMap.size();
          }));
      for (auto queueId = 0; queueId < numUcastQueues; ++queueId) {
        // Stats for all queues must be present
        EXPECT_TRUE(std::all_of(
            queueStatMaps.begin(),
            queueStatMaps.end(),
            [queueId](const auto& queueStatMap) {
              return queueStatMap.find(queueId) != queueStatMap.end();
            }));
      }
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
