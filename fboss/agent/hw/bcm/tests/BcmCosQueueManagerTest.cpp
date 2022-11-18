/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"

#include "fboss/agent/hw/bcm/tests/BcmCosQueueManagerTest.h"

#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"

#include <folly/logging/xlog.h>

using namespace facebook::fboss;

namespace facebook::fboss {

void BcmCosQueueManagerTest::checkCosQueueAPI() {
  bcm_cos_queue_t getcq;
  auto port = getHwSwitch()->getPortTable()->getBcmPort(
      PortID(masterLogicalPortIds()[0]));

  getcq = port->getQueueManager()->getCosQueue(cfg::StreamType::UNICAST, 0);
  EXPECT_EQ(getcq, BCM_GPORT_UCAST_QUEUE_GROUP_QID_GET(0));
  getcq = port->getQueueManager()->getCosQueue(cfg::StreamType::MULTICAST, 0);
  EXPECT_EQ(getcq, BCM_COSQ_GPORT_MCAST_EGRESS_QUEUE_GET(0));
  EXPECT_THROW(
      port->getQueueManager()->getCosQueue(cfg::StreamType::ALL, 0),
      FbossError);
  EXPECT_THROW(
      utility::getDefaultControlPlaneQueueSettings(
          utility::BcmChip::TOMAHAWK, static_cast<cfg::StreamType>(10)),
      FbossError);
}

void BcmCosQueueManagerTest::checkDefaultCosqMatch(
    const std::shared_ptr<PortQueue>& queue) {
  auto defQueue = std::make_shared<PortQueue>(queue->getID());
  defQueue->setStreamType(queue->getStreamType());
  if (queue->getName().has_value()) {
    defQueue->setName(queue->getName().value());
  }
  if (const auto& portQueueRate = queue->getPortQueueRate()) {
    defQueue->setPortQueueRate(portQueueRate->toThrift());
  }
  if (*queue != *defQueue) {
    XLOG(ERR) << "actual queue=" << queue->toString()
              << ", default queue=" << defQueue->toString();
  }
  EXPECT_TRUE(*queue == *defQueue);
}

void BcmCosQueueManagerTest::checkConfSwHwMatch() {
  const QueueConfig& swQueues = getSwQueues();
  // for non-warm boot case, check whether s/w matches conf
  if (getHwSwitch()->getBootType() != BootType::WARM_BOOT) {
    for (const auto& swQueue : swQueues) {
      auto cfgQueue = getCfgQueue(swQueue->getID());
      if (cfgQueue) {
        // if queue is set in conf, then check whether conf queue matches s/w
        EXPECT_TRUE(checkSwConfPortQueueMatch(swQueue, &cfgQueue.value()));
      } else {
        // otherwise, s/w queue should use default queue
        checkDefaultCosqMatch(swQueue);
      }
    }
  }

  // then check whether s/w matches h/w
  const QueueConfig& hwQueues = getHwQueues();
  EXPECT_EQ(swQueues.size(), hwQueues.size());
  for (const auto& swQueue : swQueues) {
    auto hwQueue = hwQueues[swQueue->getID()];
    // hardware queue doesn't come with the name
    if (swQueue->getName().has_value()) {
      hwQueue->setName(swQueue->getName().value());
    }
    if (*swQueue != *hwQueue) {
      XLOG(ERR) << "sw queue=" << swQueue->toString()
                << ", actual hw queue=" << hwQueue->toString();
    }
    // We can just use EXPECT_TRUE(*queue == *defQueue) but to make the debug
    // easier, we specifically call EXPECT_TRUE for all the necessary field
    EXPECT_TRUE(swQueue->getID() == hwQueue->getID());
    EXPECT_TRUE(swQueue->getStreamType() == hwQueue->getStreamType());
    EXPECT_TRUE(swQueue->getWeight() == hwQueue->getWeight());
    EXPECT_TRUE(swQueue->getReservedBytes() == hwQueue->getReservedBytes());
    EXPECT_TRUE(swQueue->getScalingFactor() == hwQueue->getScalingFactor());
    EXPECT_TRUE(swQueue->getScheduling() == hwQueue->getScheduling());
    EXPECT_TRUE(swQueue->isAqmsSame(hwQueue.get()));
    EXPECT_TRUE(swQueue->isPortQueueRateSame(hwQueue.get()));
  }
}

} // namespace facebook::fboss
