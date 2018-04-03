/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <memory>
#include <thread>
#include <vector>

#include <folly/Function.h>
#include <folly/Synchronized.h>
#include <folly/synchronization/Baton.h>
#include <folly/system/ThreadName.h>
#include <string>

#include "fboss/agent/LacpController.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/types.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using namespace std::string_literals;

namespace {

class LacpTest : public ::testing::Test {
 protected:
  LacpTest() : lacpEvb_(false) {}

  void SetUp() override {
    lacpThread_.reset(new std::thread([this] {
      static auto const name = "testLACPthread"s;
      folly::setThreadName(name);
      google::setThreadName(name);

      // TODO(samank): why doesn't this trip the CHECK...
      lacpEvb_.loopForever();
    }));
  }

  folly::EventBase* lacpEvb() {
    lacpEvb_.waitUntilRunning();
    return &lacpEvb_;
  }

  void TearDown() override {
    lacpEvb_.terminateLoopSoon();
    lacpThread_->join();
  }

 private:
  folly::EventBase lacpEvb_;
  std::unique_ptr<std::thread> lacpThread_{nullptr};
};

// This may eventually be subsumed by an LacpBehaviorInterceptor and the
// appropriate factory methods
class LacpTransmissionInterceptor : public LacpServicerIf {
 public:
  explicit LacpTransmissionInterceptor(
      folly::Function<bool(LACPDU, PortID)> transmitInterceptor)
      : transmitInterceptor_(std::move(transmitInterceptor)) {}

  // The following methods implement the LacpServicerIf interface.
  bool transmit(LACPDU lacpdu, PortID portID) override {
    return transmitInterceptor_(lacpdu, portID);
  }
  void enableForwarding(PortID portID, AggregatePortID aggPortID) override {
    // no-op
    LOG(INFO) << "Enabling member " << portID << " in "
              << " aggregate" << aggPortID;
  }
  void disableForwarding(PortID portID, AggregatePortID aggPortID) override {
    // no-op
    LOG(INFO) << "Disabling member " << portID << " in "
              << " aggregate" << aggPortID;
  }
  std::vector<std::shared_ptr<LacpController>> getControllersFor(
      folly::Range<std::vector<PortID>::const_iterator> /* ports */) override {
    return {controller_};
  }

  // This is an artifact of the circular dependencies in the test below
  void setController(std::shared_ptr<LacpController> controller) {
    controller_ = controller;
  }

 private:
  folly::Function<bool(LACPDU, PortID)> transmitInterceptor_;
  std::shared_ptr<LacpController> controller_{nullptr};

  // Force users to implement the transmit method
  LacpTransmissionInterceptor();
};

class MockLacpServicer : public LacpServicerIf {
  MOCK_METHOD2(transmit, bool(LACPDU, PortID));
  MOCK_METHOD2(enableForwarding, void(PortID, AggregatePortID));
  MOCK_METHOD2(disableForwarding, void(PortID, AggregatePortID));
  MOCK_METHOD1(
      getControllersFor,
      std::vector<std::shared_ptr<LacpController>>(
          folly::Range<std::vector<PortID>::const_iterator>));
};
} // namespace

/*
 * If a port is not aggregatable, then the frames transmitted from the
 * corresponding LacpController must _not_ have their AGGREGATABLE bit on
 */
TEST_F(LacpTest, nonAggregatablePortTransmitsIndividualBit) {
  // The LacpController constructor reserved for non-AGGREGATABLE ports takes 3
  // parameters. In what follows, we construct two of those three parameters:
  // LacpServiceIf* and folly::EventBase*.

  folly::Synchronized<LACPDU> lacpduTransmitted;
  auto onTransmission = [&lacpduTransmitted](LACPDU lacpdu, PortID portID) {
    // Cache the information we're interested in testing
    *(lacpduTransmitted.wlock()) = lacpdu;

    // "Transmit" the frame
    LOG(INFO) << "Transmitting " << lacpdu.describe() << " out port " << portID;
    return true;
  };

  // A LacpTransmissionInterceptor takes the place of a LinkAggregationManager
  // by implementing the LacpServicerIf interface.
  LacpTransmissionInterceptor transmissionInterceptor(onTransmission);

  // An LacpController takes an EventBase over which to execute. This EventBase
  // is provided by the test fixture's lacpEvb() method.

  // An LacpController's methods assume it is being managed by a shared_ptr
  auto controllerPtr = std::make_shared<LacpController>(
      PortID(1), lacpEvb(), &transmissionInterceptor);
  transmissionInterceptor.setController(controllerPtr);

  // At this point, the LacpController we would like to test and the
  // LacpTransmissionInterceptor we will use to test it with are in place
  controllerPtr->startMachines();

  // We are going to send the LacpController an LACPDU so as to induce its
  // response
  controllerPtr->received(LACPDU());

  // By scheduling this check on the same EventBase that LacpController
  // is executing on, we ensure that the LacpController has finished executing.
  // This may become overly tricky with more complicated tests.
  LacpState actorInfoTransmitted;
  lacpEvb()->runInEventBaseThreadAndWait(
      [&actorInfoTransmitted, &lacpduTransmitted]() {
        actorInfoTransmitted = lacpduTransmitted.rlock()->actorInfo.state;
      });

  // Finally, we can check that the transmitted frame did indeed signal
  // itself as not AGGREGATABLE.
  ASSERT_EQ(actorInfoTransmitted & LacpState::AGGREGATABLE, LacpState::NONE);

  controllerPtr->stopMachines();
}

/*
 * When link aggregation was first deployed to production, the following
 * logical sequence of events was observed:
 * 1. Port P's PHY transitions to DOWN
 * 2. P's Receive finite-state machine transitions to ReceiveMachine::DISABLED
 * 3. A frame is received on the downed port P
 * 4. P's Receive finite-state machine transitions to ReceiveMachine::CURRENT
 * 5. Port P's PHY transitions to UP
 * 6. CHECK_EQ(state_, ReceiveState::DISABLED) in ReceiveMachine::portUp() fails
 * because state_ == ReceiveState::CURRENT
 *
 * In physical reality, (3) had occured before (1), but because port event
 * notification and LACP frame reception occur on the update thread and RX
 * thread, respectively, and they may take different paths through lower
 * layers (eg., Linux kernel, ASIC SDK), the port down event and frame reception
 * had been interleaved so as to appear that a LACP frame was received on a down
 * port, thereby causing the violation in (6).
 *
 */
TEST_F(LacpTest, frameReceivedOnDownPort) {
  MockLacpServicer lacpServicer;

  // An LacpController's methods assume it is being managed by a shared_ptr
  auto controllerPtr =
      std::make_shared<LacpController>(PortID(1), lacpEvb(), &lacpServicer);

  // At this point, the LacpController we would like to test is in place
  controllerPtr->startMachines();

  // so we can proceed to replicate the interleaving described above

  // (1) induce a port down event
  controllerPtr->portDown();

  // (3) induce a frame reception event
  controllerPtr->received(LACPDU());

  // (5) induce a port up event
  controllerPtr->portUp();

  // If commit D6184510/15324eb480b1 has indeed corrected this edge-case, then
  // we should get here without the failed CHECK in (6)

  controllerPtr->stopMachines();
}
