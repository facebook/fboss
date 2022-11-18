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
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <boost/container/flat_map.hpp>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_shared;
using std::shared_ptr;

namespace {
constexpr auto kNumCPUQueues = 48;

cfg::Range getRange(uint32_t minimum, uint32_t maximum) {
  cfg::Range range;
  range.minimum() = minimum;
  range.maximum() = maximum;

  return range;
}

cfg::PortQueueRate getPortQueueRatePps(uint32_t minimum, uint32_t maximum) {
  cfg::PortQueueRate portQueueRate;
  portQueueRate.pktsPerSec_ref() = getRange(minimum, maximum);

  return portQueueRate;
}

std::vector<cfg::PortQueue> getConfigCPUQueues() {
  std::vector<cfg::PortQueue> cpuQueues;
  cfg::PortQueue high;
  *high.id() = 9;
  high.name() = "cpuQueue-high";
  *high.streamType() = cfg::StreamType::MULTICAST;
  *high.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  high.weight() = 4;
  cpuQueues.push_back(high);

  cfg::PortQueue mid;
  *mid.id() = 2;
  mid.name() = "cpuQueue-mid";
  *mid.streamType() = cfg::StreamType::MULTICAST;
  *mid.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  mid.weight() = 2;
  cpuQueues.push_back(mid);

  cfg::PortQueue defaultQ;
  *defaultQ.id() = 1;
  defaultQ.name() = "cpuQueue-default";
  *defaultQ.streamType() = cfg::StreamType::MULTICAST;
  *defaultQ.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  defaultQ.weight() = 1;
  defaultQ.portQueueRate() = cfg::PortQueueRate();
  defaultQ.portQueueRate()->pktsPerSec_ref() = getRange(0, 200);
  defaultQ.reservedBytes() = 1040;
  defaultQ.sharedBytes() = 10192;
  cpuQueues.push_back(defaultQ);

  cfg::PortQueue low;
  *low.id() = 0;
  low.name() = "cpuQueue-low";
  *low.streamType() = cfg::StreamType::MULTICAST;
  *low.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  low.weight() = 1;
  low.portQueueRate() = cfg::PortQueueRate();
  low.portQueueRate()->pktsPerSec_ref() = getRange(0, 100);
  low.reservedBytes() = 1040;
  low.sharedBytes() = 10192;
  cpuQueues.push_back(low);
  return cpuQueues;
}

QueueConfig genCPUQueues() {
  QueueConfig queues;
  shared_ptr<PortQueue> high = make_shared<PortQueue>(static_cast<uint8_t>(9));
  high->setName("cpuQueue-high");
  high->setStreamType(cfg::StreamType::MULTICAST);
  high->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  high->setWeight(4);
  queues.push_back(high);

  shared_ptr<PortQueue> mid = make_shared<PortQueue>(static_cast<uint8_t>(2));
  mid->setName("cpuQueue-mid");
  mid->setStreamType(cfg::StreamType::MULTICAST);
  mid->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  mid->setWeight(2);
  queues.push_back(mid);

  shared_ptr<PortQueue> defaultQ =
      make_shared<PortQueue>(static_cast<uint8_t>(1));
  defaultQ->setName("cpuQueue-default");
  defaultQ->setStreamType(cfg::StreamType::MULTICAST);
  defaultQ->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  defaultQ->setWeight(1);
  defaultQ->setPortQueueRate(getPortQueueRatePps(0, 200));
  defaultQ->setReservedBytes(1040);
  defaultQ->setSharedBytes(10192);
  queues.push_back(defaultQ);

  shared_ptr<PortQueue> low = make_shared<PortQueue>(static_cast<uint8_t>(0));
  low->setName("cpuQueue-low");
  low->setStreamType(cfg::StreamType::MULTICAST);
  low->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  low->setWeight(1);
  low->setPortQueueRate(getPortQueueRatePps(0, 100));
  low->setReservedBytes(1040);
  low->setSharedBytes(10192);
  queues.push_back(low);

  return queues;
}

boost::container::flat_map<int, shared_ptr<PortQueue>> getCPUQueuesMap() {
  QueueConfig queues = genCPUQueues();
  boost::container::flat_map<int, shared_ptr<PortQueue>> queueMap;
  for (const auto& queue : queues) {
    queueMap.emplace(queue->getID(), queue);
  }
  return queueMap;
}

shared_ptr<ControlPlane> generateControlPlane() {
  shared_ptr<ControlPlane> controlPlane = make_shared<ControlPlane>();

  auto cpuQueues = genCPUQueues();
  controlPlane->resetQueues(cpuQueues);

  ControlPlane::RxReasonToQueue reasons = {
      ControlPlane::makeRxReasonToQueueEntry(cfg::PacketRxReason::ARP, 9),
      ControlPlane::makeRxReasonToQueueEntry(cfg::PacketRxReason::DHCP, 2),
      ControlPlane::makeRxReasonToQueueEntry(cfg::PacketRxReason::BPDU, 2),
      ControlPlane::makeRxReasonToQueueEntry(cfg::PacketRxReason::UNMATCHED, 1),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::L3_MTU_ERROR, 0),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::L3_SLOW_PATH, 0),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::L3_DEST_MISS, 0),
      ControlPlane::makeRxReasonToQueueEntry(cfg::PacketRxReason::TTL_1, 0),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::CPU_IS_NHOP, 0)};
  controlPlane->resetRxReasonToQueue(reasons);

  std::optional<std::string> qosPolicy("qp1");
  controlPlane->resetQosPolicy(qosPolicy);

  return controlPlane;
}

shared_ptr<SwitchState> genCPUSwitchState() {
  auto stateV0 = make_shared<SwitchState>();
  auto cpu = make_shared<ControlPlane>();
  // the default unconfigured cpu queue settings will be obtained during init.
  QueueConfig cpuQueues;
  for (uint8_t i = 0; i < kNumCPUQueues; i++) {
    auto queue = std::make_shared<PortQueue>(i);
    queue->setStreamType(cfg::StreamType::MULTICAST);
    cpuQueues.push_back(queue);
  }
  cpu->resetQueues(cpuQueues);
  stateV0->resetControlPlane(cpu);

  return stateV0;
}
} // namespace

TEST(ControlPlane, serialize) {
  auto controlPlane = generateControlPlane();
  // to folly dynamic
  auto serialized = controlPlane->toFollyDynamic();
  // back to ControlPlane object
  auto controlPlaneBack = ControlPlane::fromFollyDynamic(serialized);
  EXPECT_TRUE(*controlPlane == *controlPlaneBack);
  validateNodeSerialization(*controlPlane);
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
  validateNodeSerialization(*controlPlane);
}

TEST(ControlPlane, applyDefaultConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = genCPUSwitchState();

  // apply default cpu 4 queues settings
  auto cfgCpuQueues = getConfigCPUQueues();
  cfg::SwitchConfig config;
  *config.cpuQueues() = cfgCpuQueues;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);

  auto newQueues = stateV1->getControlPlane()->getQueues();
  // it should always generate all queues
  EXPECT_EQ(newQueues.size(), kNumCPUQueues);
  auto cpu4QueuesMap = getCPUQueuesMap();
  for (const auto& queue : newQueues) {
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
  validateNodeSerialization(*stateV1->getControlPlane());
}

TEST(ControlPlane, applySameConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = genCPUSwitchState();

  // apply default cpu 4 queues settings
  auto cfgCpuQueues = getConfigCPUQueues();
  cfg::SwitchConfig config;
  *config.cpuQueues() = cfgCpuQueues;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  validateNodeSerialization(*stateV1->getControlPlane());

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_EQ(nullptr, stateV2);
}

TEST(ControlPlane, resetLowPrioQueue) {
  auto platform = createMockPlatform();
  auto stateV0 = genCPUSwitchState();

  // apply default cpu 4 queues settings
  auto cfgCpuQueues = getConfigCPUQueues();
  cfg::SwitchConfig config;
  *config.cpuQueues() = cfgCpuQueues;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  validateNodeSerialization(*stateV1->getControlPlane());

  auto newCfgCpuQueues = getConfigCPUQueues();
  newCfgCpuQueues.erase(newCfgCpuQueues.begin() + 3);
  cfg::SwitchConfig newConfig;
  *newConfig.cpuQueues() = newCfgCpuQueues;
  auto stateV2 = publishAndApplyConfig(stateV1, &newConfig, platform.get());
  EXPECT_NE(nullptr, stateV2);
  validateNodeSerialization(*stateV2->getControlPlane());

  auto newQueues = stateV2->getControlPlane()->getQueues();
  // it should always generate all queues
  EXPECT_EQ(newQueues.size(), kNumCPUQueues);
  auto cpu4QueuesMap = getCPUQueuesMap();
  // low-prio has been removed
  cpu4QueuesMap.erase(cpu4QueuesMap.find(0));
  for (const auto& queue : newQueues) {
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
  *config.cpuQueues() = cfgCpuQueues;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  validateNodeSerialization(*stateV1->getControlPlane());

  auto newCfgCpuQueues = getConfigCPUQueues();
  // change low queue pps from 100 to 1000. the last one is low queue
  auto& lowQueue = newCfgCpuQueues.at(newCfgCpuQueues.size() - 1);
  lowQueue.portQueueRate() = cfg::PortQueueRate();
  lowQueue.portQueueRate()->pktsPerSec_ref() = getRange(0, 1000);

  cfg::SwitchConfig newConfig;
  *newConfig.cpuQueues() = newCfgCpuQueues;
  auto stateV2 = publishAndApplyConfig(stateV1, &newConfig, platform.get());
  EXPECT_NE(nullptr, stateV2);
  validateNodeSerialization(*stateV2->getControlPlane());

  auto newQueues = stateV2->getControlPlane()->getQueues();
  // it should always generate all queues
  EXPECT_EQ(newQueues.size(), kNumCPUQueues);
  auto cpu4QueuesMap = getCPUQueuesMap();
  // low-prio has been changed(pps from 100->1000)
  cpu4QueuesMap.find(0)->second->setPortQueueRate(getPortQueueRatePps(0, 1000));

  for (const auto& queue : newQueues) {
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
  for (const auto& cfgQueue : cfgCpuQueues) {
    auto swQueueItr = swCpuQueuesMap.find(*cfgQueue.id());
    EXPECT_NE(swQueueItr, swCpuQueuesMap.end());
    EXPECT_TRUE(checkSwConfPortQueueMatch(swQueueItr->second, &cfgQueue));
  }
}

TEST(ControlPlane, testRxReasonToQueueBackwardsCompat) {
  auto platform = createMockPlatform();
  auto stateV0 = genCPUSwitchState();

  cfg::SwitchConfig config;
  config.cpuTrafficPolicy() = cfg::CPUTrafficPolicyConfig();
  // only set old version of mapping (map instead of ordereded list)
  config.cpuTrafficPolicy()->rxReasonToCPUQueue() = {
      {cfg::PacketRxReason::ARP, 9}};
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(stateV1, nullptr);
  validateNodeSerialization(*stateV1->getControlPlane());

  const auto reasonToQueue1 = stateV1->getControlPlane()->getRxReasonToQueue();
  EXPECT_EQ(reasonToQueue1.size(), 1);
  const auto entry1 = reasonToQueue1.at(0);
  EXPECT_EQ(*entry1.rxReason(), cfg::PacketRxReason::ARP);
  EXPECT_EQ(*entry1.queueId(), 9);
}
