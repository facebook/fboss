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
#include "fboss/agent/hw/HwPortFb303Stats.h"
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
    setupStage = SetupStage::PORT | SetupStage::QUEUE;
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
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(10));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {4};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles =
      saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
  saiManagerTable->queueManager().ensurePortQueueConfig(
      portSaiId, queueHandles, queueConfig);
  checkQueue(queueHandles, portSaiId, streamType, {queueIds});
}

TEST_F(QueueManagerTest, loadDuplicateQueue) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(10));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {4};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles1 =
      saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
  saiManagerTable->queueManager().ensurePortQueueConfig(
      portSaiId, queueHandles1, queueConfig);
  checkQueue(queueHandles1, portSaiId, streamType, {queueIds});
  auto queueHandles2 =
      saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
  saiManagerTable->queueManager().ensurePortQueueConfig(
      portSaiId, queueHandles2, queueConfig);
  checkQueue(queueHandles2, portSaiId, streamType, {queueIds});
  compareQueueHandles(queueHandles1, queueHandles2);
}

TEST_F(QueueManagerTest, loadMultipleQueues) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(10));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles =
      saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
  saiManagerTable->queueManager().ensurePortQueueConfig(
      portSaiId, queueHandles, queueConfig);
  checkQueue(queueHandles, portSaiId, streamType, {queueIds});
}

TEST_F(QueueManagerTest, removeQueues) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(10));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  {
    auto queueHandles =
        saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
    saiManagerTable->queueManager().ensurePortQueueConfig(
        portSaiId, queueHandles, queueConfig);
    checkQueue(queueHandles, portSaiId, streamType, {queueIds});
  }
  // Queues will not be unloaded till the ports are deleted.
  for (auto queueId : queueIds) {
    auto saiQueueConfig = makeSaiQueueConfig(cfg::StreamType::UNICAST, queueId);
    auto queueHandle = saiManagerTable->portManager().getQueueHandle(
        PortID(10), saiQueueConfig);
    EXPECT_TRUE(queueHandle);
  }
}

TEST_F(QueueManagerTest, checkNonExistentQueues) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(10));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  std::vector<uint8_t> nonExistentQueueIds = {7, 8, 10};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles =
      saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
  saiManagerTable->queueManager().ensurePortQueueConfig(
      portSaiId, queueHandles, queueConfig);
  checkQueue(queueHandles, portSaiId, streamType, {queueIds});
  for (auto queueId : nonExistentQueueIds) {
    auto saiQueueConfig = makeSaiQueueConfig(cfg::StreamType::UNICAST, queueId);
    auto queueHandle = saiManagerTable->portManager().getQueueHandle(
        PortID(10), saiQueueConfig);
    EXPECT_FALSE(queueHandle);
  }
}

TEST_F(QueueManagerTest, getNonExistentQueues) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(10));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  std::vector<uint8_t> nonExistentQueueIds = {7, 8, 10};
  auto queueConfig = makeQueueConfig({queueIds});
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto queueHandles =
      saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
  saiManagerTable->queueManager().ensurePortQueueConfig(
      portSaiId, queueHandles, queueConfig);
  checkQueue(queueHandles, portSaiId, streamType, {queueIds});
  for (auto queueId : nonExistentQueueIds) {
    auto saiQueueConfig = makeSaiQueueConfig(cfg::StreamType::UNICAST, queueId);
    auto queueHandle = saiManagerTable->portManager().getQueueHandle(
        PortID(10), saiQueueConfig);
    EXPECT_FALSE(queueHandle);
  }
}

TEST_F(QueueManagerTest, loadUCMCQueueWithSameQueueId) {
  auto portHandle = saiManagerTable->portManager().getPortHandle(PortID(10));
  PortSaiId portSaiId = portHandle->port->adapterKey();
  // Fake has queue id 6 allocated for unicast and multicast
  std::vector<uint8_t> queueIds = {6};
  auto ucQueueConfig = makeQueueConfig({queueIds}, cfg::StreamType::UNICAST);
  auto mcQueueConfig = makeQueueConfig({queueIds}, cfg::StreamType::MULTICAST);
  auto queueSaiIds = getPortQueueSaiIds(portHandle);
  auto ucQueueHandles =
      saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
  saiManagerTable->queueManager().ensurePortQueueConfig(
      portSaiId, ucQueueHandles, ucQueueConfig);
  auto mcQueueHandles =
      saiManagerTable->queueManager().loadQueues(portSaiId, queueSaiIds);
  saiManagerTable->queueManager().ensurePortQueueConfig(
      portSaiId, mcQueueHandles, mcQueueConfig);
  checkQueue(ucQueueHandles, portSaiId, cfg::StreamType::UNICAST, {queueIds});
  checkQueue(mcQueueHandles, portSaiId, cfg::StreamType::MULTICAST, {queueIds});
}

TEST_F(QueueManagerTest, changePortQueue) {
  auto p0 = testInterfaces[0].remoteHosts[0].port;
  std::shared_ptr<Port> oldPort = makePort(p0);
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 3, 5};
  auto queueConfig = makeQueueConfig({queueIds});
  auto newPort = oldPort->clone();
  newPort->resetPortQueues(queueConfig);
  saiManagerTable->portManager().changePort(oldPort, newPort);

  auto newNewPort = newPort->clone();
  std::vector<uint8_t> newQueueIds = {3, 4, 5, 6};
  auto newQueueConfig = makeQueueConfig({newQueueIds});
  newNewPort->resetPortQueues(newQueueConfig);
  saiManagerTable->portManager().changePort(newPort, newNewPort);

  auto portHandle =
      saiManagerTable->portManager().getPortHandle(newNewPort->getID());
  PortSaiId portSaiId = portHandle->port->adapterKey();

  checkQueue(portHandle->queues, portSaiId, streamType, {newQueueIds});
}

TEST_F(QueueManagerTest, addPortQueueAndCheckStats) {
  auto p0 = testInterfaces[0].remoteHosts[0].port;
  std::shared_ptr<Port> swPort = makePort(p0);
  auto newPort = swPort->clone();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  auto queueConfig = makeQueueConfig({queueIds});
  newPort->resetPortQueues(queueConfig);
  saiManagerTable->portManager().changePort(swPort, newPort);
  saiManagerTable->portManager().updateStats();
  auto portStat =
      saiManagerTable->portManager().getLastPortStat(swPort->getID());
  for (const auto& portQueue : queueConfig) {
    for (auto statKey : HwPortFb303Stats::kQueueStatKeys()) {
      EXPECT_TRUE(
          facebook::fbData->getStatMap()->contains(HwPortFb303Stats::statName(
              statKey,
              swPort->getName(),
              portQueue->getID(),
              *portQueue->getName())));
      EXPECT_EQ(
          portStat->getCounterLastIncrement(HwPortFb303Stats::statName(
              statKey,
              swPort->getName(),
              portQueue->getID(),
              *portQueue->getName())),
          0);
    }
  }
}

TEST_F(QueueManagerTest, removePortQueueAndCheckQueueStats) {
  auto p0 = testInterfaces[0].remoteHosts[0].port;
  std::shared_ptr<Port> oldPort = makePort(p0);
  auto newPort = oldPort->clone();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  auto queueConfig = makeQueueConfig({queueIds});
  newPort->resetPortQueues(queueConfig);
  saiManagerTable->portManager().changePort(oldPort, newPort);
  newPort->publish();
  auto newNewPort = newPort->clone();
  std::vector<uint8_t> newQueueIds = {1, 2};
  auto newQueueConfig = makeQueueConfig({newQueueIds});
  newNewPort->resetPortQueues(newQueueConfig);
  saiManagerTable->portManager().changePort(newPort, newNewPort);
  saiManagerTable->portManager().updateStats();
  auto portStat =
      saiManagerTable->portManager().getLastPortStat(newPort->getID());
  for (const auto& portQueue : newQueueConfig) {
    for (auto statKey : HwPortFb303Stats::kQueueStatKeys()) {
      EXPECT_TRUE(
          facebook::fbData->getStatMap()->contains(HwPortFb303Stats::statName(
              statKey,
              newNewPort->getName(),
              portQueue->getID(),
              *portQueue->getName())));
      EXPECT_EQ(
          portStat->getCounterLastIncrement(HwPortFb303Stats::statName(
              statKey,
              newNewPort->getName(),
              portQueue->getID(),
              *portQueue->getName())),
          0);
    }
  }
  for (auto queueId : {3, 4}) {
    for (auto statKey : HwPortFb303Stats::kQueueStatKeys()) {
      EXPECT_FALSE(
          facebook::fbData->getStatMap()->contains(HwPortFb303Stats::statName(
              statKey,
              newNewPort->getName(),
              queueId,
              folly::to<std::string>("queue", queueId))));
    }
  }
}

TEST_F(QueueManagerTest, changePortQueueNameAndCheckStats) {
  auto p0 = testInterfaces[0].remoteHosts[0].port;
  std::shared_ptr<Port> oldPort = makePort(p0);
  auto newPort = oldPort->clone();
  auto streamType = cfg::StreamType::UNICAST;
  std::vector<uint8_t> queueIds = {1};
  auto queueConfig = makeQueueConfig({queueIds});
  newPort->resetPortQueues(queueConfig);
  saiManagerTable->portManager().changePort(oldPort, newPort);
  saiManagerTable->portManager().updateStats();
  auto newNewPort = newPort->clone();
  queueConfig[0]->setName("portQueue");
  newNewPort->resetPortQueues(queueConfig);
  saiManagerTable->portManager().changePort(newPort, newNewPort);
  auto portStat =
      saiManagerTable->portManager().getLastPortStat(newNewPort->getID());
  for (auto statKey : HwPortFb303Stats::kQueueStatKeys()) {
    EXPECT_TRUE(
        facebook::fbData->getStatMap()->contains(HwPortFb303Stats::statName(
            statKey, newNewPort->getName(), 1, "portQueue")));
    EXPECT_FALSE(
        facebook::fbData->getStatMap()->contains(HwPortFb303Stats::statName(
            statKey, newNewPort->getName(), 1, "queue1")));
    EXPECT_EQ(
        portStat->getCounterLastIncrement(HwPortFb303Stats::statName(
            statKey, oldPort->getName(), 1, "portQueue")),
        0);
  }
}
