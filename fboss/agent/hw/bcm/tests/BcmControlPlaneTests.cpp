/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmCosQueueManagerTest.h"

#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"

using namespace facebook::fboss;

namespace {
const auto kLowCpuQueueWeight = 1;
const auto kLowCpuQueueReservedMmuCellNum = 5;
const auto kLowCpuQueueSharedMmuCellNum = 50;

std::vector<cfg::PortQueue> getConfigCPUQueues(uint8_t mmuCellBytes) {
  std::vector<cfg::PortQueue> cpuQueues;
  cfg::PortQueue high;
  high.id = 9;
  high.name_ref() = "cpuQueue-high";
  high.streamType = cfg::StreamType::MULTICAST;
  high.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  high.weight_ref() = 4;
  cpuQueues.push_back(high);

  cfg::PortQueue mid;
  mid.id = 2;
  mid.name_ref() = "cpuQueue-mid";
  mid.streamType = cfg::StreamType::MULTICAST;
  mid.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  mid.weight_ref() = 2;
  cpuQueues.push_back(mid);

  cfg::PortQueue defaultQ;
  defaultQ.id = 1;
  defaultQ.name_ref() = "cpuQueue-default";
  defaultQ.streamType = cfg::StreamType::MULTICAST;
  defaultQ.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  defaultQ.weight_ref() = 1;
  defaultQ.portQueueRate_ref() = cfg::PortQueueRate();
  defaultQ.portQueueRate_ref()->set_pktsPerSec(utility::getRange(0, 200));
  defaultQ.reservedBytes_ref() = 5 * mmuCellBytes;
  defaultQ.sharedBytes_ref() = 50 * mmuCellBytes;
  cpuQueues.push_back(defaultQ);

  cfg::PortQueue low;
  low.id = 0;
  low.name_ref() = "cpuQueue-low";
  low.streamType = cfg::StreamType::MULTICAST;
  low.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  low.weight_ref() = kLowCpuQueueWeight;
  low.portQueueRate_ref() = cfg::PortQueueRate();
  low.portQueueRate_ref()->set_pktsPerSec(utility::getRange(0, 100));
  low.reservedBytes_ref() = kLowCpuQueueReservedMmuCellNum * mmuCellBytes;
  low.sharedBytes_ref() = kLowCpuQueueSharedMmuCellNum * mmuCellBytes;
  cpuQueues.push_back(low);
  return cpuQueues;
}
} // namespace

namespace facebook::fboss {

class BcmControlPlaneTest : public BcmCosQueueManagerTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }

  std::optional<cfg::PortQueue> getCfgQueue(int queueID) override {
    std::optional<cfg::PortQueue> cfgQueue{std::nullopt};
    for (const auto& queue : programmedCfg_.cpuQueues) {
      if (queue.id == queueID) {
        cfgQueue = queue;
        break;
      }
    }
    return cfgQueue;
  }

  QueueConfig getHwQueues() override {
    return getHwSwitch()->getControlPlane()->getMulticastQueueSettings();
  }

  const QueueConfig& getSwQueues() override {
    return getProgrammedState()->getControlPlane()->getQueues();
  }
};

} // namespace facebook::fboss

TEST_F(BcmControlPlaneTest, DefaultCPUQueuesCheckWithoutConfig) {
  if (getHwSwitch()->getBootType() == BootType::WARM_BOOT) {
    return;
  }
  // if we've never set cosq configs to any queue. The system should come up
  // with all default values.
  const auto& queues =
      getHwSwitch()->getControlPlane()->getMulticastQueueSettings();

  // default queue settings
  for (const auto& queue : queues) {
    checkDefaultCosqMatch(queue);
  }
}

TEST_F(BcmControlPlaneTest, ConfigCPUQueuesSetup) {
  auto setup = [=]() {
    auto cfg = initialConfig();
    cfg.cpuQueues = getConfigCPUQueues(getPlatform()->getMMUCellBytes());
    applyNewConfig(cfg);
  };

  auto verify = [&]() { checkConfSwHwMatch(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmControlPlaneTest, ChangeCPULowQueueSettings) {
  QueueConfig swQueuesBefore;
  auto setup = [&]() {
    auto cfg = initialConfig();
    // first program the default cpu queues
    cfg.cpuQueues = getConfigCPUQueues(getPlatform()->getMMUCellBytes());
    applyNewConfig(cfg);

    for (const auto& queue : getSwQueues()) {
      swQueuesBefore.push_back(queue);
    }

    // change low queue pps from 100 to 1000. the last one is low queue
    auto& lowQueue = cfg.cpuQueues.at(cfg.cpuQueues.size() - 1);
    lowQueue.portQueueRate_ref() = cfg::PortQueueRate();
    lowQueue.portQueueRate_ref()->set_pktsPerSec(utility::getRange(0, 1000));

    applyNewConfig(cfg);
  };

  auto verify = [&]() {
    checkConfSwHwMatch();

    const auto swQueuesAfter = getSwQueues();
    EXPECT_EQ(swQueuesBefore.size(), swQueuesAfter.size());

    // now explicitly check low-prio
    auto lowQ = swQueuesAfter[0];
    EXPECT_EQ(lowQ->getID(), 0);
    EXPECT_EQ(lowQ->getWeight(), kLowCpuQueueWeight);
    EXPECT_EQ(
        lowQ->getReservedBytes().value(),
        kLowCpuQueueReservedMmuCellNum * getPlatform()->getMMUCellBytes());
    EXPECT_EQ(
        lowQ->getSharedBytes().value(),
        kLowCpuQueueSharedMmuCellNum * getPlatform()->getMMUCellBytes());

    EXPECT_EQ(
        lowQ->getPortQueueRate().value().getType(),
        cfg::PortQueueRate::Type::pktsPerSec);
    EXPECT_EQ(lowQ->getPortQueueRate().value().get_pktsPerSec().minimum, 0);
    EXPECT_EQ(lowQ->getPortQueueRate().value().get_pktsPerSec().maximum, 1000);

    // other queues shouldn't be affected
    for (int i = 1; i < swQueuesAfter.size(); i++) {
      EXPECT_TRUE(*swQueuesBefore[i] == *swQueuesAfter[i]);
    }
  };

  setup();
  verify();
}

TEST_F(BcmControlPlaneTest, DisableCPULowQueueSettings) {
  QueueConfig swQueuesBefore;
  auto setup = [&]() {
    auto cfg = initialConfig();
    // first program the default cpu queues
    cfg.cpuQueues = getConfigCPUQueues(getPlatform()->getMMUCellBytes());
    applyNewConfig(cfg);

    for (const auto& queue : getSwQueues()) {
      swQueuesBefore.push_back(queue);
    }

    // remove the low-prio queue
    cfg.cpuQueues.erase(cfg.cpuQueues.begin() + 3);
    applyNewConfig(cfg);
  };

  auto verify = [&]() {
    checkConfSwHwMatch();

    const auto swQueuesAfter = getSwQueues();
    EXPECT_EQ(swQueuesBefore.size(), swQueuesAfter.size());
    // now check low-prio, it should be using default setting
    auto lowQ = swQueuesAfter[0];
    checkDefaultCosqMatch(lowQ);

    // other queues shouldn't be affected
    for (int i = 1; i < swQueuesAfter.size(); i++) {
      EXPECT_TRUE(*swQueuesBefore[i] == *swQueuesAfter[i]);
    }
  };

  setup();
  verify();
}
