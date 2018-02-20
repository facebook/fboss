/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/state/ControlPlane.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/SwitchState.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_shared;
using std::shared_ptr;

shared_ptr<ControlPlane> generateDefaultControlPlane() {
  shared_ptr<ControlPlane> controlPlane = make_shared<ControlPlane>();

  ControlPlane::CPUQueueConfig queues;
  shared_ptr<PortQueue> high = make_shared<PortQueue>(9);
  high->setStreamType(cfg::StreamType::MULTICAST);
  high->setWeight(4);
  queues.push_back(high);

  shared_ptr<PortQueue> mid = make_shared<PortQueue>(2);
  mid->setStreamType(cfg::StreamType::MULTICAST);
  mid->setWeight(2);
  queues.push_back(mid);

  shared_ptr<PortQueue> defaultQ = make_shared<PortQueue>(1);
  defaultQ->setStreamType(cfg::StreamType::MULTICAST);
  defaultQ->setWeight(1);
  defaultQ->setReservedBytes(1000);
  queues.push_back(defaultQ);

  shared_ptr<PortQueue> low = make_shared<PortQueue>(0);
  low->setStreamType(cfg::StreamType::MULTICAST);
  low->setWeight(1);
  low->setReservedBytes(1000);
  queues.push_back(low);

  controlPlane->resetQueues(queues);

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


TEST(ControlPlane, serialize) {
  auto controlPlane = generateDefaultControlPlane();
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

  controlPlane = generateDefaultControlPlane();
  state->resetControlPlane(controlPlane);
  // modify unpublished state
  EXPECT_EQ(controlPlane.get(), controlPlane->modify(&state));

  controlPlane->publish();
  auto modifiedCP = controlPlane->modify(&state);
  EXPECT_NE(controlPlane.get(), modifiedCP);
  EXPECT_TRUE(*controlPlane == *modifiedCP);
}
