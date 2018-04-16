/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <folly/MacAddress.h>
#include <folly/Function.h>
#include <folly/Synchronized.h>
#include <folly/Range.h>
#include <folly/synchronization/Baton.h>
#include <folly/system/ThreadName.h>
#include <string>

#include "fboss/agent/LacpController.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
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

class LacpServiceInterceptor : public LacpServicerIf {
 public:
  explicit LacpServiceInterceptor(folly::EventBase* lacpEvb)
      : lacpEvb_(lacpEvb) {}

  // The following methods implement the LacpServicerIf interface
  bool transmit(LACPDU lacpdu, PortID portID) override {
    *(lastTransmission_.wlock()) = lacpdu;

    // "Transmit" the frame
    LOG(INFO) << "Transmitting " << lacpdu.describe() << " out port " << portID;
    return true;
  }
  void enableForwarding(PortID portID, AggregatePortID aggPortID) override {
    LOG(INFO) << "Enabling member " << portID << " in "
              << "aggregate " << aggPortID;

    // "Enable" forwarding
    *(isForwarding_.wlock()) = true;
  }
  void disableForwarding(PortID portID, AggregatePortID aggPortID) override {
    LOG(INFO) << "Disabling member " << portID << " in "
              << "aggregate " << aggPortID;

    // "Disable" forwarding"
    *(isForwarding_.wlock()) = false;
  }
  std::vector<std::shared_ptr<LacpController>> getControllersFor(
      folly::Range<std::vector<PortID>::const_iterator> /* ports */) override {
    return {controller_};
  }

  // setController is needed because of a circular dependency between the
  void setController(std::shared_ptr<LacpController> controller) {
    controller_ = controller;
  }

  // The following methods ensure all processing up to their invocation has
  // has completed in the LACP eventbase.
  LacpState lastActorStateTransmitted() {
    LacpState actorStateTransmitted = LacpState::NONE;

    lacpEvb_->runInEventBaseThreadAndWait([this, &actorStateTransmitted]() {
      actorStateTransmitted = lastTransmission_.rlock()->actorInfo.state;
    });

    return actorStateTransmitted;
  }
  LacpState lastPartnerStateTransmitted() {
    LacpState partnerStateTransmitted = LacpState::NONE;

    lacpEvb_->runInEventBaseThreadAndWait([this, &partnerStateTransmitted]() {
      partnerStateTransmitted = lastTransmission_.rlock()->partnerInfo.state;
    });

    return partnerStateTransmitted;
  }
  bool isForwarding() {
    bool forwarding = false;

    lacpEvb_->runInEventBaseThreadAndWait([this, &forwarding]() {
      forwarding = *(isForwarding_.rlock());
    });

    return forwarding;
  }

 private:
  std::shared_ptr<LacpController> controller_{nullptr};
  folly::Synchronized<bool> isForwarding_{false};
  folly::Synchronized<LACPDU> lastTransmission_;
  folly::EventBase* lacpEvb_{nullptr};
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
  // parameters. In what follows, we construct those three parameters:
  // PortID, LacpServiceIf*, and folly::EventBase*.

  // A LacpServiceInterceptor takes the place of a LinkAggregationManager by
  // implementing the LacpServicerIf interface.
  LacpServiceInterceptor serviceInterceptor(lacpEvb());

  // An LacpController takes an EventBase over which to execute. This EventBase
  // is provided by the test fixture's lacpEvb() method.

  // An LacpController's methods assume it is being managed by a shared_ptr
  auto controllerPtr = std::make_shared<LacpController>(
      PortID(1), lacpEvb(), &serviceInterceptor);
  serviceInterceptor.setController(controllerPtr);

  // At this point, the LacpController we would like to test and the
  // LacpServiceInterceptor we will use to test it with are in place
  controllerPtr->startMachines();

  // We are going to send the LacpController an LACPDU so as to induce its
  // response
  controllerPtr->received(LACPDU());

  // Finally, we can check that the transmitted frame did indeed signal
  // itself as not AGGREGATABLE.
  ASSERT_EQ(
      serviceInterceptor.lastActorStateTransmitted() & LacpState::AGGREGATABLE,
      LacpState::NONE);

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

TEST_F(LacpTest, DUColdBootReconvergenceWithESW) {
  LacpServiceInterceptor duEventInterceptor(lacpEvb());

  // The following declares some attributes of the DU and the ESW derived from
  // configuration
  ParticipantInfo duInfo;
  duInfo.systemPriority = 65535;
  // TODO(samank): check endianness
  duInfo.systemID = {{0x02, 0x90, 0xfb, 0x5e, 0x1e, 0x8d}};
  duInfo.key = 903;
  duInfo.portPriority = 32768;
  duInfo.port = 42;

  ParticipantInfo eswInfo;
  eswInfo.systemPriority = 32768;
  // TODO(samank): check endianness
  eswInfo.systemID = {{0x00, 0x1c, 0x73, 0x5b, 0xa8, 0x47}};
  eswInfo.key = 609;
  eswInfo.portPriority = 32768;
  eswInfo.port = 499;

  // Some bits in LacpState are derived from configuration. To avoid having to
  // repreat them, they are factored out into the following variables
  const LacpState eswActorStateBase =
      LacpState::AGGREGATABLE | LacpState::ACTIVE | LacpState::SHORT_TIMEOUT;
  const LacpState duActorStateBase =
      LacpState::AGGREGATABLE | LacpState::ACTIVE | LacpState::SHORT_TIMEOUT;

  // duController contains the totality of the LACP logic we would like to test
  // Although its lifetime coincides with this stack frame, duController's
  // methods assume it is being managed by a shared_ptr
  auto duControllerPtr = std::make_shared<LacpController>(
      PortID(duInfo.port),
      lacpEvb(),
      duInfo.portPriority,
      cfg::LacpPortRate::FAST,
      cfg::LacpPortActivity::ACTIVE,
      AggregatePortID(duInfo.key),
      duInfo.systemPriority,
      MacAddress::fromBinary(
          folly::ByteRange(duInfo.systemID.cbegin(), duInfo.systemID.cend())),
      1 /* minimum-link count */,
      &duEventInterceptor);

  duEventInterceptor.setController(duControllerPtr);

  // The following four variables are used for ease of reading
  LacpState eswActorState = LacpState::NONE;
  LacpState eswPartnerState = LacpState::NONE;

  duControllerPtr->startMachines();

  duControllerPtr->portUp();

  /*** First Round Trip ***/

  // The following declares the state of the ESW _after_ the agent has come up
  // on the DU but before any LACP data-units have been exchanged.

  // After a DU cold-boot, the ESW has timed out through multiple
  // current_while_timer's, so it's DEFAULTED
  eswActorState = eswActorStateBase | LacpState::DEFAULTED;
  // The ESW knows nothing about its partner
  eswPartnerState = LacpState::SHORT_TIMEOUT;

  /* actorInfo=(SystemPriority 32768,
   *            SystemID 00:1c:73:5b:a8:47,
   *            Key 609,
   *            PortPriority 32768,
   *            Port 499,
   *            State 199)
   */
  ParticipantInfo eswActorInfo = eswInfo;
  eswActorInfo.state = eswActorState;
  /* partnerInfo=(SystemPriority 0,
   *              SystemID 00:00:00:00:00:00,
   *              Key 0,
   *              PortPriority 0,
   *              Port 0,
   *              State 2)
   */
  ParticipantInfo eswPartnerInfo = ParticipantInfo::defaultParticipantInfo();
  eswPartnerInfo.state = eswPartnerState;

  // The ESW transmits the first LACP data-unit
  duControllerPtr->received(LACPDU(eswActorInfo, eswPartnerInfo));

  ASSERT_EQ(
      duEventInterceptor.lastActorStateTransmitted(),
      duActorStateBase | LacpState::IN_SYNC);
  // What the partner told us about itself should be reflected back
  ASSERT_EQ(duEventInterceptor.lastPartnerStateTransmitted(), eswActorState);

  /*** Second Round Trip ***/
  eswActorState = eswActorStateBase | LacpState::IN_SYNC;
  eswPartnerState = duActorStateBase | LacpState::IN_SYNC;

  /* actorInfo=(SystemPriority 32768,
   *            SystemID 00:1c:73:5b:a8:47,
   *            Key 609,
   *            PortPriority 32768,
   *            Port 499,
   *            State 15)
   */
  eswActorInfo = eswInfo;
  eswActorInfo.state = eswActorState;

  /*
   * partnerInfo=(SystemPriority 65535,
   *              SystemID 02:90:fb:5e:1e:8d,
   *              Key 903,
   *              PortPriority 32768,
   *              Port 42,
   *              State 15)
   */
  eswPartnerInfo = duInfo;
  eswPartnerInfo.state = eswPartnerState;

  duControllerPtr->received(LACPDU(eswActorInfo, eswPartnerInfo));

  ASSERT_EQ(
      duEventInterceptor.lastActorStateTransmitted(),
      duActorStateBase | LacpState::IN_SYNC |
      LacpState::COLLECTING | LacpState::DISTRIBUTING);
  // What the partner told us about iself should be reflected back
  ASSERT_EQ(
      duEventInterceptor.lastPartnerStateTransmitted(), eswActorInfo.state);
  ASSERT(duEventInterceptor.isForwarding());

  /*** Third Round Trip ***/
  eswActorState = eswActorStateBase | LacpState::IN_SYNC |
      LacpState::COLLECTING | LacpState::DISTRIBUTING;
  eswPartnerState = duActorStateBase | LacpState::IN_SYNC |
      LacpState::COLLECTING | LacpState::DISTRIBUTING;

  /*
   * actorInfo=(SystemPriority 32768,
   *            SystemID 00:1c:73:5b:a8:47,
   *            Key 609,
   *            PortPriority 32768,
   *            Port 499,
   *            State 63)
   */
  eswActorInfo = eswInfo;
  eswActorInfo.state = eswActorState;

  /*
   * partnerInfo=(SystemPriority 65535,
   *              SystemID 02:90:fb:5e:1e:8d,
   *              Key 903,
   *              PortPriority 32768,
   *              Port 42,
   *              State 63)
   */
  eswPartnerInfo = duInfo;
  eswPartnerInfo.state = eswPartnerState;

  duControllerPtr->received(LACPDU(eswActorInfo, eswPartnerInfo));

  // TODO(samank): how would a PASSIVE LACP speaker handle this?
  // Waiting double the PeriodicTransmissionMachine's tranmission period
  // guarantees a new LACP frame has been transmitted
  std::this_thread::sleep_for(PeriodicTransmissionMachine::SHORT_PERIOD * 2);

  ASSERT_EQ(
      duEventInterceptor.lastActorStateTransmitted(),
      duActorStateBase | LacpState::IN_SYNC |
      LacpState::COLLECTING | LacpState::DISTRIBUTING);
  ASSERT_EQ(
      duEventInterceptor.lastPartnerStateTransmitted(),
      eswActorInfo.state);
  // TODO(samank): really we should check that forwarding hasn't changed, even
  // twice
  ASSERT(duEventInterceptor.isForwarding());

  duControllerPtr->stopMachines();
}
