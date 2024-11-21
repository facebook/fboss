// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/soak_tests/InterruptTest.h"

#include <thread>

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"

#include "common/process/Process.h"

namespace facebook::fboss {

void InterruptTest::setUpPorts() {
  SwSwitch* swSwitch = sw();
  PortID firstPortID = PortID(
      utility::getFirstMap(swSwitch->getState()->getPorts())->cbegin()->first);
  XLOG(DBG2) << " Enable mac loopback on the first port " << firstPortID;
  setPortLoopbackMode(firstPortID, cfg::PortLoopbackMode::MAC);
  frontPanelPortToLoopTraffic_ = firstPortID;
}

void InterruptTest::SetUp() {
  AgentTest::SetUp();
  setUpPorts();

  originalIntrTimeout_ = platform()->getIntrTimeout();

  XLOG(DBG2) << "The original intr_timeout is " << originalIntrTimeout_;

  // Set the timeout to be 1 sec, so the scheduling latency should not be a
  // factor. If we see any non-zero intr_timeout_count, it should be from real
  // hw interrupt miss.
  platform()->setIntrTimeout(1000000);

  XLOG(DBG2) << "Soak Test setup ready";
}

void InterruptTest::TearDown() {
  // Undo the intr_timeout setting change done in SetUp()
  platform()->setIntrTimeout(originalIntrTimeout_);

  AgentTest::TearDown();
}

bool InterruptTest::RunOneLoop(SoakLoopArgs* args) {
  InterruptLoopArgs* intrArgs = static_cast<InterruptLoopArgs*>(args);
  int numPktsPerLoop = intrArgs->numPktsPerLoop;

  XLOG(DBG5) << "numPktsPerLoop = " << numPktsPerLoop;

  SwSwitch* swSwitch = sw();
  uint64_t intrTimeoutCountStart = platform()->getIntrTimeoutCount();
  uint64_t intrCountStart = platform()->getIntrCount();

  auto vlan =
      utility::getFirstMap(swSwitch->getState()->getVlans())->cbegin()->second;
  auto scope = swSwitch->getScopeResolver()->scope(vlan);
  auto switchId = scope.switchId();
  utility::pumpTraffic(
      true, // is IPv6
      utility::getAllocatePktFn(sw()),
      utility::getSendPktFunc(sw()),
      swSwitch->getLocalMac(switchId),
      vlan->getID(),
      frontPanelPortToLoopTraffic_);

  uint64_t intrTimeoutCountEnd = platform()->getIntrTimeoutCount();

  EXPECT_EQ(intrTimeoutCountStart, intrTimeoutCountEnd);

  uint64_t intrCountEnd = platform()->getIntrCount();

  uint64_t intrCountDiff = intrCountEnd - intrCountStart;
  XLOG(DBG2) << "intr_count = " << intrCountEnd << ", diff = " << intrCountDiff;

  // The interrupt counts should be over 0.  It will fail for the platforms
  // which have not implemented the interrupt counts functions.  At this point,
  // this works for TH3 only.
  EXPECT_GT(intrCountDiff, 0);

  return true;
}

TEST_F(InterruptTest, IntrMiss) {
  XLOG(DBG2) << "In test case IntrMiss";

  InterruptLoopArgs args;
  args.numPktsPerLoop = 1;

  bool ret = SoakTest::RunLoops(&args);

  EXPECT_EQ(ret, true);
}

} // namespace facebook::fboss
