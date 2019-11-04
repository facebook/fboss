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
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiQueueManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/types.h"

#include <string>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class QueueManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT;
    ManagerTestBase::SetUp();
  }

  SaiQueueConfig makeSaiQueueConfig(
      cfg::StreamType streamType,
      uint8_t queueId) {
    return std::make_pair(queueId, streamType);
  }

  void checkQueue(
      const SaiQueueHandles& queueHandles,
      PortSaiId saiPortId,
      cfg::StreamType streamType,
      std::vector<uint8_t>& queueIds) {
    auto& queueApi = saiApiTable->queueApi();
    for (auto queueId : queueIds) {
      auto saiQueueConfig = makeSaiQueueConfig(streamType, queueId);
      auto queueHandle = queueHandles.find(saiQueueConfig);
      EXPECT_TRUE(queueHandle != queueHandles.end());
      QueueSaiId queueSaiId = queueHandle->second->queue->adapterKey();
      SaiQueueTraits::Attributes::Type type;
      auto gotType = queueApi.getAttribute(queueSaiId, type);
      SaiQueueTraits::Attributes::Port port;
      auto gotPort = queueApi.getAttribute(queueSaiId, port);
      SaiQueueTraits::Attributes::Index queueIndex;
      auto gotQueueId = queueApi.getAttribute(queueSaiId, queueIndex);
      auto expectedType = streamType == cfg::StreamType::UNICAST
          ? SAI_QUEUE_TYPE_UNICAST
          : SAI_QUEUE_TYPE_MULTICAST;
      EXPECT_EQ(gotType, expectedType);
      EXPECT_EQ(gotPort, saiPortId);
      EXPECT_EQ(gotQueueId, queueId);
    }
  }

  void compareQueueHandles(
      const SaiQueueHandles& queueHandles1,
      const SaiQueueHandles& queueHandles2) {
    EXPECT_EQ(queueHandles1.size(), queueHandles2.size());
    EXPECT_NE(queueHandles1.size(), 0);
    EXPECT_NE(queueHandles2.size(), 0);
    for (auto& queueHandle1 : queueHandles1) {
      auto queueHandle2 = queueHandles2.find(queueHandle1.first);
      EXPECT_TRUE(queueHandle2 != queueHandles2.end());
      EXPECT_EQ(queueHandle1.second->queue, queueHandle2->second->queue);
    }
  }
};

TEST_F(QueueManagerTest, loadQueue) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(1));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {4};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles = saiManagerTable->queueManager().loadQueues(
      portSaiId, queueSaiIds, queueConfig);
  checkQueue(queueHandles, portSaiId, streamType, {queueIds});
}

TEST_F(QueueManagerTest, loadDuplicateQueue) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(1));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {4};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles1 = saiManagerTable->queueManager().loadQueues(
      portSaiId, queueSaiIds, queueConfig);
  checkQueue(queueHandles1, portSaiId, streamType, {queueIds});
  auto queueHandles2 = saiManagerTable->queueManager().loadQueues(
      portSaiId, queueSaiIds, queueConfig);
  checkQueue(queueHandles2, portSaiId, streamType, {queueIds});
  compareQueueHandles(queueHandles1, queueHandles2);
}

TEST_F(QueueManagerTest, loadMultipleQueues) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(1));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles = saiManagerTable->queueManager().loadQueues(
      portSaiId, queueSaiIds, queueConfig);
  checkQueue(queueHandles, portSaiId, streamType, {queueIds});
}

TEST_F(QueueManagerTest, removeQueues) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(1));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  {
    auto queueHandles = saiManagerTable->queueManager().loadQueues(
        portSaiId, queueSaiIds, queueConfig);
    checkQueue(queueHandles, portSaiId, streamType, {queueIds});
  }
  for (auto queueId : queueIds) {
    auto saiQueueConfig = makeSaiQueueConfig(cfg::StreamType::UNICAST, queueId);
    auto queueHandle = saiManagerTable->portManager().getQueueHandle(
        PortID(1), saiQueueConfig);
    EXPECT_FALSE(queueHandle);
  }
}

TEST_F(QueueManagerTest, checkNonExistentQueues) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(1));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  std::vector<uint8_t> nonExistentQueueIds = {7, 8, 10};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles = saiManagerTable->queueManager().loadQueues(
      portSaiId, queueSaiIds, queueConfig);
  checkQueue(queueHandles, portSaiId, streamType, {queueIds});
  for (auto queueId : nonExistentQueueIds) {
    auto saiQueueConfig = makeSaiQueueConfig(cfg::StreamType::UNICAST, queueId);
    auto queueHandle = saiManagerTable->portManager().getQueueHandle(
        PortID(1), saiQueueConfig);
    EXPECT_FALSE(queueHandle);
  }
}

TEST_F(QueueManagerTest, getNonExistentQueues) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(1));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  std::vector<uint8_t> nonExistentQueueIds = {7, 8, 10};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles = saiManagerTable->queueManager().loadQueues(
      portSaiId, queueSaiIds, queueConfig);
  checkQueue(queueHandles, portSaiId, streamType, {queueIds});
  for (auto queueId : nonExistentQueueIds) {
    auto saiQueueConfig = makeSaiQueueConfig(cfg::StreamType::UNICAST, queueId);
    auto queueHandle = saiManagerTable->portManager().getQueueHandle(
        PortID(1), saiQueueConfig);
    EXPECT_FALSE(queueHandle);
  }
}

TEST_F(QueueManagerTest, loadUCMCQueueWithSameQueueId) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(1));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  // Fake has queue id 6 allocated for unicast and multicast
  std::vector<uint8_t> queueIds = {6};
  auto ucQueueConfig = makeQueueConfig({queueIds}, cfg::StreamType::UNICAST);
  auto mcQueueConfig = makeQueueConfig({queueIds}, cfg::StreamType::MULTICAST);
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto ucQueueHandles = saiManagerTable->queueManager().loadQueues(
      portSaiId, queueSaiIds, ucQueueConfig);
  auto mcQueueHandles = saiManagerTable->queueManager().loadQueues(
      portSaiId, queueSaiIds, mcQueueConfig);
  checkQueue(ucQueueHandles, portSaiId, cfg::StreamType::UNICAST, {queueIds});
  checkQueue(mcQueueHandles, portSaiId, cfg::StreamType::MULTICAST, {queueIds});
}
