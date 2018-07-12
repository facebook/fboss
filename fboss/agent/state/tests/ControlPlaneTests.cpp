/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/SwitchState.h"

#include <boost/container/flat_map.hpp>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_shared;
using std::shared_ptr;

namespace {
constexpr auto kNumCPUQueues = 48;

std::vector<cfg::PortQueue> getConfigCPUQueues() {
  std::vector<cfg::PortQueue> cpuQueues;
  cfg::PortQueue high;
  high.id = 9;
  high.name = "cpuQueue-high";
  high.__isset.name = true;
  high.streamType = cfg::StreamType::MULTICAST;
  high.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  high.weight = 4;
  high.__isset.weight = true;
  cpuQueues.push_back(high);

  cfg::PortQueue mid;
  mid.id = 2;
  mid.name = "cpuQueue-mid";
  mid.__isset.name = true;
  mid.streamType = cfg::StreamType::MULTICAST;
  mid.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  mid.weight = 2;
  mid.__isset.weight = true;
  cpuQueues.push_back(mid);

  cfg::PortQueue defaultQ;
  defaultQ.id = 1;
  defaultQ.name = "cpuQueue-default";
  defaultQ.__isset.name = true;
  defaultQ.streamType = cfg::StreamType::MULTICAST;
  defaultQ.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  defaultQ.weight = 1;
  defaultQ.__isset.weight = true;
  defaultQ.packetsPerSec = 200;
  defaultQ.__isset.packetsPerSec = true;
  defaultQ.reservedBytes = 1040;
  defaultQ.__isset.reservedBytes = true;
  defaultQ.sharedBytes = 10192;
  defaultQ.__isset.sharedBytes = true;
  cpuQueues.push_back(defaultQ);

  cfg::PortQueue low;
  low.id = 0;
  low.name = "cpuQueue-low";
  low.__isset.name = true;
  low.streamType = cfg::StreamType::MULTICAST;
  low.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  low.weight = 1;
  low.__isset.weight = true;
  low.packetsPerSec = 100;
  low.__isset.packetsPerSec = true;
  low.reservedBytes = 1040;
  low.__isset.reservedBytes = true;
  low.sharedBytes = 10192;
  low.__isset.sharedBytes = true;
  cpuQueues.push_back(low);
  return cpuQueues;
}

QueueConfig genCPUQueues() {
  QueueConfig queues;
  shared_ptr<PortQueue> high = make_shared<PortQueue>(9);
  high->setName("cpuQueue-high");
  high->setStreamType(cfg::StreamType::MULTICAST);
  high->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  high->setWeight(4);
  queues.push_back(high);

  shared_ptr<PortQueue> mid = make_shared<PortQueue>(2);
  mid->setName("cpuQueue-mid");
  mid->setStreamType(cfg::StreamType::MULTICAST);
  mid->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  mid->setWeight(2);
  queues.push_back(mid);

  shared_ptr<PortQueue> defaultQ = make_shared<PortQueue>(1);
  defaultQ->setName("cpuQueue-default");
  defaultQ->setStreamType(cfg::StreamType::MULTICAST);
  defaultQ->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  defaultQ->setWeight(1);
  defaultQ->setPacketsPerSec(200);
  defaultQ->setReservedBytes(1040);
  defaultQ->setSharedBytes(10192);
  queues.push_back(defaultQ);

  shared_ptr<PortQueue> low = make_shared<PortQueue>(0);
  low->setName("cpuQueue-low");
  low->setStreamType(cfg::StreamType::MULTICAST);
  low->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  low->setWeight(1);
  low->setPacketsPerSec(100);
  low->setReservedBytes(1040);
  low->setSharedBytes(10192);
  queues.push_back(low);

  return queues;
}

boost::container::flat_map<int, shared_ptr<PortQueue>> getCPUQueuesMap() {
  QueueConfig queues = genCPUQueues();
  boost::container::flat_map<int, shared_ptr<PortQueue>> queueMap;
  for (const auto& queue: queues) {
    queueMap.emplace(queue->getID(), queue);
  }
  return queueMap;
}

shared_ptr<ControlPlane> generateControlPlane() {
  shared_ptr<ControlPlane> controlPlane = make_shared<ControlPlane>();

  auto cpuQueues = genCPUQueues();
  controlPlane->resetQueues(cpuQueues);

  ControlPlane::RxReasonToQueue reasons = {
   {cfg::PacketRxReason::ARP, 9},
   {cfg::PacketRxReason::DHCP, 2},
   {cfg::PacketRxReason::BPDU, 2},
   {cfg::PacketRxReason::UNMATCHED, 1},
   {cfg::PacketRxReason::L3_SLOW_PATH, 0},
   {cfg::PacketRxReason::L3_DEST_MISS, 0},
   {cfg::PacketRxReason::TTL_1, 0},
   {cfg::PacketRxReason::CPU_IS_NHOP, 0}
  };
  controlPlane->resetRxReasonToQueue(reasons);

  return controlPlane;
}

shared_ptr<SwitchState> genCPUSwitchState() {
  auto stateV0 = make_shared<SwitchState>();
  auto cpu = make_shared<ControlPlane>();
  // the default unconfigured cpu queue settings will be obtained during init.
  QueueConfig cpuQueues;
  for (int i = 0; i < kNumCPUQueues; i++) {
    auto queue = std::make_shared<PortQueue>(i);
    queue->setStreamType(cfg::StreamType::MULTICAST);
    cpuQueues.push_back(queue);
  }
  cpu->resetQueues(cpuQueues);
  stateV0->resetControlPlane(cpu);

  return stateV0;
}
}


TEST(ControlPlane, serialize) {
  auto controlPlane = generateControlPlane();
  // to folly dynamic
  auto serialized = controlPlane->toFollyDynamic();
  // back to ControlPlane object
  auto controlPlaneBack = ControlPlane::fromFollyDynamic(serialized);
  EXPECT_TRUE(*controlPlane == *controlPlaneBack);
}

TEST(ControlPlane, modify) {
  auto state = make_shared<SwitchState>();
  auto controlPlane = state->getControlPlane();
  // modify unpublished state
  EXPECT_EQ(controlPlane.get(), controlPlane->modify(&state));

  controlPlane = generateControlPlane();
  state->resetControlPlane(controlPlane);
  // modify unpublished state
  EXPECT_EQ(controlPlane.get(), controlPlane->modify(&state));

  controlPlane->publish();
  auto modifiedCP = controlPlane->modify(&state);
  EXPECT_NE(controlPlane.get(), modifiedCP);
  EXPECT_TRUE(*controlPlane == *modifiedCP);
}

TEST(ControlPlane, applyDefaultConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = genCPUSwitchState();

  // apply default cpu 4 queues settings
  auto cfgCpuQueues = getConfigCPUQueues();
  cfg::SwitchConfig config;
  config.cpuQueues = cfgCpuQueues;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);

  auto newQueues = stateV1->getControlPlane()->getQueues();
  // it should always generate all queues
  EXPECT_EQ(newQueues.size(), kNumCPUQueues);
  auto cpu4QueuesMap = getCPUQueuesMap();
  for (const auto& queue: newQueues) {
    if (cpu4QueuesMap.find(queue->getID()) == cpu4QueuesMap.end()) {
      // if it's not one of those 4 queues, it should have default value
      auto unconfiguredQueue = std::make_shared<PortQueue>(queue->getID());
      unconfiguredQueue->setStreamType(cfg::StreamType::MULTICAST);
      EXPECT_TRUE(*unconfiguredQueue == *queue);
    } else {
      auto& cpuQueue = cpu4QueuesMap.find(queue->getID())->second;
      EXPECT_TRUE(*cpuQueue == *queue);
    }
  }
}

TEST(ControlPlane, applySameConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = genCPUSwitchState();

  // apply default cpu 4 queues settings
  auto cfgCpuQueues = getConfigCPUQueues();
  cfg::SwitchConfig config;
  config.cpuQueues = cfgCpuQueues;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_EQ(nullptr, stateV2);
}

TEST(ControlPlane, resetLowPrioQueue) {
  auto platform = createMockPlatform();
  auto stateV0 = genCPUSwitchState();

  // apply default cpu 4 queues settings
  auto cfgCpuQueues = getConfigCPUQueues();
  cfg::SwitchConfig config;
  config.cpuQueues = cfgCpuQueues;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);

  auto newCfgCpuQueues = getConfigCPUQueues();
  newCfgCpuQueues.erase(newCfgCpuQueues.begin() + 3);
  cfg::SwitchConfig newConfig;
  newConfig.cpuQueues = newCfgCpuQueues;
  auto stateV2 = publishAndApplyConfig(stateV1, &newConfig, platform.get());
  EXPECT_NE(nullptr, stateV2);

  auto newQueues = stateV2->getControlPlane()->getQueues();
  // it should always generate all queues
  EXPECT_EQ(newQueues.size(), kNumCPUQueues);
  auto cpu4QueuesMap = getCPUQueuesMap();
  // low-prio has been removed
  cpu4QueuesMap.erase(cpu4QueuesMap.find(0));
  for (const auto& queue: newQueues) {
    if (cpu4QueuesMap.find(queue->getID()) == cpu4QueuesMap.end()) {
      // if it's not one of those 4 queues, it should have default value
      // also since low-prio has been removed, it should be checked in here.
      auto unconfiguredQueue = std::make_shared<PortQueue>(queue->getID());
      unconfiguredQueue->setStreamType(cfg::StreamType::MULTICAST);
      EXPECT_TRUE(*unconfiguredQueue == *queue);
    } else {
      auto& cpuQueue = cpu4QueuesMap.find(queue->getID())->second;
      EXPECT_TRUE(*cpuQueue == *queue);
    }
  }
}

TEST(ControlPlane, changeLowPrioQueue) {
  auto platform = createMockPlatform();
  auto stateV0 = genCPUSwitchState();

  // apply default cpu 4 queues settings
  auto cfgCpuQueues = getConfigCPUQueues();
  cfg::SwitchConfig config;
  config.cpuQueues = cfgCpuQueues;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);

  auto newCfgCpuQueues = getConfigCPUQueues();
  // change low queue pps from 100 to 1000. the last one is low queue
  auto& lowQueue = newCfgCpuQueues.at(newCfgCpuQueues.size() - 1);
  lowQueue.packetsPerSec = 1000;
  cfg::SwitchConfig newConfig;
  newConfig.cpuQueues = newCfgCpuQueues;
  auto stateV2 = publishAndApplyConfig(stateV1, &newConfig, platform.get());
  EXPECT_NE(nullptr, stateV2);

  auto newQueues = stateV2->getControlPlane()->getQueues();
  // it should always generate all queues
  EXPECT_EQ(newQueues.size(), kNumCPUQueues);
  auto cpu4QueuesMap = getCPUQueuesMap();
  // low-prio has been changed(pps from 100->1000)
  cpu4QueuesMap.find(0)->second->setPacketsPerSec(1000);
  for (const auto& queue: newQueues) {
    if (cpu4QueuesMap.find(queue->getID()) == cpu4QueuesMap.end()) {
      // if it's not one of those 4 queues, it should have default value
      auto unconfiguredQueue = std::make_shared<PortQueue>(queue->getID());
      unconfiguredQueue->setStreamType(cfg::StreamType::MULTICAST);
      EXPECT_TRUE(*unconfiguredQueue == *queue);
    } else {
      auto& cpuQueue = cpu4QueuesMap.find(queue->getID())->second;
      EXPECT_TRUE(*cpuQueue == *queue);
    }
  }
}

TEST(ControlPlane, checkSwConfPortQueueMatch) {
  auto cfgCpuQueues = getConfigCPUQueues();
  auto swCpuQueuesMap = getCPUQueuesMap();
  for (const auto& cfgQueue: cfgCpuQueues) {
    auto swQueueItr = swCpuQueuesMap.find(cfgQueue.id);
    EXPECT_NE(swQueueItr, swCpuQueuesMap.end());
    EXPECT_TRUE(checkSwConfPortQueueMatch(swQueueItr->second, &cfgQueue));
  }
}
