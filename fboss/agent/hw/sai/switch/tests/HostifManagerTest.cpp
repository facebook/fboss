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
#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"

#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include <mutex>

using namespace facebook::fboss;

class HostifManagerTest : public ManagerTestBase {};

TEST_F(HostifManagerTest, createHostifTrap) {
  uint32_t queueId = 4;
  auto trapType = cfg::PacketRxReason::ARP;
  HostifTrapSaiId trapId =
      saiManagerTable->hostifManager().addHostifTrap(trapType, queueId, 1);
  auto trapTypeExpected = saiApiTable->hostifApi().getAttribute(
      trapId, SaiHostifTrapTraits::Attributes::TrapType{});
  auto trapTypeCfg = SaiHostifManager::packetReasonToHostifTrap(trapType);
  EXPECT_EQ(trapTypeCfg, trapTypeExpected);
}

TEST_F(HostifManagerTest, sharedHostifTrapGroup) {
  uint32_t queueId = 4;
  auto trapType1 = cfg::PacketRxReason::ARP;
  auto trapType2 = cfg::PacketRxReason::DHCP;
  auto& hostifManager = saiManagerTable->hostifManager();
  HostifTrapSaiId trapId1 = hostifManager.addHostifTrap(trapType1, queueId, 1);
  HostifTrapSaiId trapId2 = hostifManager.addHostifTrap(trapType2, queueId, 2);
  auto trapGroup1 = saiApiTable->hostifApi().getAttribute(
      trapId1, SaiHostifTrapTraits::Attributes::TrapGroup{});
  auto trapGroup2 = saiApiTable->hostifApi().getAttribute(
      trapId2, SaiHostifTrapTraits::Attributes::TrapGroup{});
  EXPECT_EQ(trapGroup1, trapGroup2);
  auto queueIdExpected = saiApiTable->hostifApi().getAttribute(
      HostifTrapGroupSaiId{trapGroup1},
      SaiHostifTrapGroupTraits::Attributes::Queue{});
  EXPECT_EQ(queueIdExpected, queueId);
}

TEST_F(HostifManagerTest, removeHostifTrap) {
  uint32_t queueId = 4;
  auto& hostifManager = saiManagerTable->hostifManager();
  auto trapType1 = cfg::PacketRxReason::ARP;
  auto trapType2 = cfg::PacketRxReason::DHCP;
  // create two traps using the same queue
  HostifTrapSaiId trapId1 = hostifManager.addHostifTrap(trapType1, queueId, 1);
  HostifTrapSaiId trapId2 = hostifManager.addHostifTrap(trapType2, queueId, 2);
  auto trapGroup1 = saiApiTable->hostifApi().getAttribute(
      trapId1, SaiHostifTrapTraits::Attributes::TrapGroup{});
  // remove one of them
  saiManagerTable->hostifManager().removeHostifTrap(trapType1);
  // trap group for the queue should still be valid
  auto trapGroup2 = saiApiTable->hostifApi().getAttribute(
      trapId2, SaiHostifTrapTraits::Attributes::TrapGroup{});
  EXPECT_EQ(trapGroup1, trapGroup2);
}

TEST_F(HostifManagerTest, addCpuQueueAndCheckStats) {
  auto prevControlPlane = std::make_shared<ControlPlane>();
  auto newControlPlane = prevControlPlane->clone();
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  auto queueConfig = makeQueueConfig({queueIds}, cfg::StreamType::ALL);
  newControlPlane->resetQueues(queueConfig);
  auto prevState = std::make_shared<SwitchState>();
  prevState->resetControlPlane(prevControlPlane);
  auto newState = std::make_shared<SwitchState>();
  newState->resetControlPlane(newControlPlane);
  auto delta = StateDelta(prevState, newState);
  saiManagerTable->hostifManager().processHostifDelta(
      delta.getControlPlaneDelta());
  saiManagerTable->hostifManager().updateStats();

  const auto& cpuStat = saiManagerTable->hostifManager().getCpuFb303Stats();
  for (const auto& portQueue : queueConfig) {
    for (auto statKey : HwCpuFb303Stats::kQueueStatKeys()) {
      EXPECT_TRUE(
          facebook::fbData->getStatMap()->contains(HwCpuFb303Stats::statName(
              statKey, portQueue->getID(), *portQueue->getName())));
      EXPECT_EQ(
          cpuStat.getCounterLastIncrement(HwCpuFb303Stats::statName(
              statKey, portQueue->getID(), *portQueue->getName())),
          0);
    }
  }
}

TEST_F(HostifManagerTest, removeCpuQueueAndCheckStats) {
  auto prevControlPlane = std::make_shared<ControlPlane>();
  auto newControlPlane = prevControlPlane->clone();
  std::vector<uint8_t> queueIds = {1, 2, 3, 4};
  auto queueConfig = makeQueueConfig({queueIds}, cfg::StreamType::ALL);
  newControlPlane->resetQueues(queueConfig);
  auto prevState = std::make_shared<SwitchState>();
  prevState->resetControlPlane(prevControlPlane);
  auto newState = std::make_shared<SwitchState>();
  newState->resetControlPlane(newControlPlane);
  auto delta0 = StateDelta(prevState, newState);
  saiManagerTable->hostifManager().processHostifDelta(
      delta0.getControlPlaneDelta());

  auto newNewControlPlane = newControlPlane->clone();
  std::vector<uint8_t> newQueueIds = {1, 2};
  auto newQueueConfig = makeQueueConfig({newQueueIds}, cfg::StreamType::ALL);
  newNewControlPlane->resetQueues(newQueueConfig);
  auto newNewState = newState->clone();
  newNewState->resetControlPlane(newNewControlPlane);
  auto delta1 = StateDelta(newState, newNewState);
  saiManagerTable->hostifManager().processHostifDelta(
      delta1.getControlPlaneDelta());
  saiManagerTable->hostifManager().updateStats();

  const auto& cpuStat = saiManagerTable->hostifManager().getCpuFb303Stats();
  for (auto queueId : {1, 2}) {
    auto queueName = folly::to<std::string>("queue", queueId);
    for (auto statKey : HwCpuFb303Stats::kQueueStatKeys()) {
      EXPECT_TRUE(facebook::fbData->getStatMap()->contains(
          HwCpuFb303Stats::statName(statKey, queueId, queueName)));
      EXPECT_EQ(
          cpuStat.getCounterLastIncrement(
              HwCpuFb303Stats::statName(statKey, queueId, queueName)),
          0);
    }
  }
  for (auto queueId : {3, 4}) {
    for (auto statKey : HwCpuFb303Stats::kQueueStatKeys()) {
      EXPECT_FALSE(
          facebook::fbData->getStatMap()->contains(HwCpuFb303Stats::statName(
              statKey, queueId, folly::to<std::string>("queue", queueId))));
    }
  }
}

TEST_F(HostifManagerTest, changeCpuQueueAndCheckStats) {
  auto prevControlPlane = std::make_shared<ControlPlane>();
  auto newControlPlane = prevControlPlane->clone();
  auto queueConfig = makeQueueConfig({1}, cfg::StreamType::ALL);
  newControlPlane->resetQueues(queueConfig);
  auto prevState = std::make_shared<SwitchState>();
  prevState->resetControlPlane(prevControlPlane);
  auto newState = std::make_shared<SwitchState>();
  newState->resetControlPlane(newControlPlane);
  auto delta0 = StateDelta(prevState, newState);
  saiManagerTable->hostifManager().processHostifDelta(
      delta0.getControlPlaneDelta());

  auto newNewControlPlane = newControlPlane->clone();
  auto newQueueConfig = makeQueueConfig({1}, cfg::StreamType::ALL);
  newQueueConfig[0]->setName("high");
  newNewControlPlane->resetQueues(newQueueConfig);
  auto newNewState = newState->clone();
  newNewState->resetControlPlane(newNewControlPlane);
  auto delta1 = StateDelta(newState, newNewState);
  saiManagerTable->hostifManager().processHostifDelta(
      delta1.getControlPlaneDelta());
  saiManagerTable->hostifManager().updateStats();

  auto oldQueueName = folly::to<std::string>("queue", 1);
  const auto& cpuStat = saiManagerTable->hostifManager().getCpuFb303Stats();
  for (auto statKey : HwCpuFb303Stats::kQueueStatKeys()) {
    EXPECT_TRUE(facebook::fbData->getStatMap()->contains(
        HwCpuFb303Stats::statName(statKey, 1, "high")));
    EXPECT_FALSE(facebook::fbData->getStatMap()->contains(
        HwCpuFb303Stats::statName(statKey, 1, oldQueueName)));
    EXPECT_EQ(
        cpuStat.getCounterLastIncrement(
            HwCpuFb303Stats::statName(statKey, 1, "high")),
        0);
  }
}

TEST_F(HostifManagerTest, checkHostifPriority) {
  auto prevControlPlane = std::make_shared<ControlPlane>();
  auto newControlPlane = prevControlPlane->clone();
  std::vector<std::pair<cfg::PacketRxReason, uint16_t>>
      rxReasonToQueueMappings = {
          std::pair(cfg::PacketRxReason::ARP, 1),
          std::pair(cfg::PacketRxReason::ARP_RESPONSE, 2),
          std::pair(cfg::PacketRxReason::BGP, 3),
      };
  std::vector<cfg::PacketRxReasonToQueue> rxReasonToQueues;
  for (auto rxEntry : rxReasonToQueueMappings) {
    auto rxReasonToQueue = cfg::PacketRxReasonToQueue();
    rxReasonToQueue.rxReason_ref() = rxEntry.first;
    rxReasonToQueue.queueId_ref() = rxEntry.second;
    rxReasonToQueues.push_back(rxReasonToQueue);
  }
  newControlPlane->resetRxReasonToQueue(rxReasonToQueues);
  auto prevState = std::make_shared<SwitchState>();
  prevState->resetControlPlane(prevControlPlane);
  auto newState = std::make_shared<SwitchState>();
  newState->resetControlPlane(newControlPlane);
  auto delta0 = StateDelta(prevState, newState);
  saiManagerTable->hostifManager().processHostifDelta(
      delta0.getControlPlaneDelta());
  int index = 0;
  for (auto rxEntry : rxReasonToQueueMappings) {
    auto hostifTrapHandle =
        saiManagerTable->hostifManager().getHostifTrapHandle(rxEntry.first);
    EXPECT_TRUE(hostifTrapHandle);
    auto priority = saiApiTable->hostifApi().getAttribute(
        hostifTrapHandle->trap->adapterKey(),
        SaiHostifTrapTraits::Attributes::TrapPriority{});
    EXPECT_EQ(priority, rxReasonToQueueMappings.size() - index++);
  }
  // Remove two traps and ensure the priority is reassigned.
  rxReasonToQueues.clear();
  auto rxReasonToQueue = cfg::PacketRxReasonToQueue();
  rxReasonToQueue.rxReason_ref() = cfg::PacketRxReason::ARP_RESPONSE;
  rxReasonToQueue.queueId_ref() = 1;
  rxReasonToQueues.push_back(rxReasonToQueue);
  auto newControlPlaneNew = prevControlPlane->clone();
  newControlPlaneNew->resetRxReasonToQueue(rxReasonToQueues);
  auto newNewState = std::make_shared<SwitchState>();
  newNewState->resetControlPlane(newControlPlaneNew);
  auto delta = StateDelta(newState, newNewState);
  saiManagerTable->hostifManager().processHostifDelta(
      delta.getControlPlaneDelta());
  auto hostifTrapHandle = saiManagerTable->hostifManager().getHostifTrapHandle(
      cfg::PacketRxReason::ARP_RESPONSE);
  EXPECT_TRUE(hostifTrapHandle);
  auto priority = saiApiTable->hostifApi().getAttribute(
      hostifTrapHandle->trap->adapterKey(),
      SaiHostifTrapTraits::Attributes::TrapPriority{});
  EXPECT_EQ(priority, 1);
}

TEST_F(HostifManagerTest, resetSchedulerOid) {
  // Create queue 1 and a scheduler config with Stream Type ALL
  auto prevControlPlane = std::make_shared<ControlPlane>();
  auto newControlPlane = prevControlPlane->clone();
  auto queueConfig = makeQueueConfig({1}, cfg::StreamType::ALL);
  newControlPlane->resetQueues(queueConfig);
  auto prevState = std::make_shared<SwitchState>();
  prevState->resetControlPlane(prevControlPlane);
  auto newState = std::make_shared<SwitchState>();
  newState->resetControlPlane(newControlPlane);

  // Apply the config changes using processHostifDelta
  auto delta = StateDelta(prevState, newState);
  saiManagerTable->hostifManager().processHostifDelta(
      delta.getControlPlaneDelta());
  const auto queueHandle = saiManagerTable->hostifManager().getQueueHandle(
      std::pair(1, cfg::StreamType::ALL));

  // Ensure scheduler is created with a valid OID.
  EXPECT_TRUE(queueHandle->scheduler);

  // Ensure queue scheduler profile id is set with a valid OID.
  auto schedulerProfileId = saiApiTable->queueApi().getAttribute(
      queueHandle->queue->adapterKey(),
      SaiQueueTraits::Attributes::SchedulerProfileId{});
  EXPECT_EQ(queueHandle->scheduler->adapterKey(), schedulerProfileId);

  // Free the scheduler and ensure queue scheduler profile OID is reset.
  queueHandle->resetQueue();
  schedulerProfileId = saiApiTable->queueApi().getAttribute(
      queueHandle->queue->adapterKey(),
      SaiQueueTraits::Attributes::SchedulerProfileId{});
  EXPECT_EQ(schedulerProfileId, SAI_NULL_OBJECT_ID);
}
