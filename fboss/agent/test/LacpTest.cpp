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
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <boost/container/flat_map.hpp>
#include <folly/Function.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/Synchronized.h>
#include <folly/logging/xlog.h>
#include <folly/synchronization/Baton.h>
#include <folly/system/ThreadName.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/LacpController.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;

namespace {

class LacpTest : public ::testing::Test {
 protected:
  LacpTest() : lacpEvb_(false) {}

  void SetUp() override {
    lacpThread_.reset(new std::thread([this] {
      folly::setThreadName("testLACPthread");

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
    auto portToLastTransmissionLocked = portToLastTransmission_.wlock();

    (*portToLastTransmissionLocked)[portID] = lacpdu;

    // "Transmit" the frame
    return true;
  }
  void enableForwarding(PortID portID, AggregatePortID aggPortID) override {
    auto portToIsForwardingLocked = portToIsForwarding_.wlock();

    (*portToIsForwardingLocked)[portID] = true;

    // "Enable" forwarding
    XLOG(INFO) << "Enabling member " << portID << " in "
               << "aggregate " << aggPortID;
  }
  void disableForwarding(PortID portID, AggregatePortID aggPortID) override {
    auto portToIsForwardingLocked = portToIsForwarding_.wlock();

    (*portToIsForwardingLocked)[portID] = false;

    // "Disable" forwarding
    XLOG(INFO) << "Disabling member " << portID << " in "
               << "aggregate " << aggPortID;
  }
  std::vector<std::shared_ptr<LacpController>> getControllersFor(
      folly::Range<std::vector<PortID>::const_iterator> ports) override {
    std::vector<std::shared_ptr<LacpController>> filteredControllers;

    for (const auto& port : ports) {
      for (const auto& controller : controllers_) {
        if (port == controller->portID()) {
          filteredControllers.push_back(controller);
        }
      }
    }

    return filteredControllers;
  }

  void addController(std::shared_ptr<LacpController> controller) {
    controllers_.push_back(controller);
  }

  // The following methods ensure all processing up to their invocation has
  // has completed in the LACP eventbase.
  LacpState lastActorStateTransmitted(PortID portID) {
    LacpState actorStateTransmitted = LacpState::NONE;

    lacpEvb_->runInEventBaseThreadAndWait(
        [this, portID, &actorStateTransmitted]() {
          auto lastTransmission = portToLastTransmission_.rlock()->at(portID);
          actorStateTransmitted = lastTransmission.actorInfo.state;
        });

    return actorStateTransmitted;
  }
  LacpState lastPartnerStateTransmitted(PortID portID) {
    LacpState partnerStateTransmitted = LacpState::NONE;

    lacpEvb_->runInEventBaseThreadAndWait(
        [this, portID, &partnerStateTransmitted]() {
          auto lastTransmission = portToLastTransmission_.rlock()->at(portID);
          partnerStateTransmitted = lastTransmission.partnerInfo.state;
        });

    return partnerStateTransmitted;
  }
  bool isForwarding(PortID portID) {
    bool forwarding = false;

    lacpEvb_->runInEventBaseThreadAndWait([this, portID, &forwarding]() {
      forwarding = portToIsForwarding_.rlock()->at(portID);
    });

    return forwarding;
  }

  ~LacpServiceInterceptor() {
    lacpEvb_->runInEventBaseThreadAndWait([this]() {
      for (auto& controller : controllers_) {
        controller.reset();
      }
    });
  }

 private:
  std::vector<std::shared_ptr<LacpController>> controllers_;

  using PortIDToForwardingStateMap = boost::container::flat_map<PortID, bool>;
  folly::Synchronized<PortIDToForwardingStateMap> portToIsForwarding_;

  using PortIDToLacpduMap = boost::container::flat_map<PortID, LACPDU>;
  folly::Synchronized<PortIDToLacpduMap> portToLastTransmission_;

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
  serviceInterceptor.addController(controllerPtr);

  // At this point, the LacpController we would like to test and the
  // LacpServiceInterceptor we will use to test it with are in place
  controllerPtr->startMachines();

  controllerPtr->portUp();

  // We are going to send the LacpController an LACPDU so as to induce its
  // response. Note that actorInfo needs to take on some bits different than
  // that of the default actor information because otherwise the
  // ReceiveMachine would not drive NeedToTransmit.
  ParticipantInfo actorInfo;
  actorInfo.state = LacpState::ACTIVE | LacpState::AGGREGATABLE;
  ParticipantInfo partnerInfo = ParticipantInfo::defaultParticipantInfo();
  controllerPtr->received(LACPDU(actorInfo, partnerInfo));

  // Finally, we can check that the transmitted frame did indeed signal
  // itself as not AGGREGATABLE.
  ASSERT_EQ(
      serviceInterceptor.lastActorStateTransmitted(PortID(1)) &
          LacpState::AGGREGATABLE,
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

  PortID duPort = static_cast<PortID>(duInfo.port);

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

  duEventInterceptor.addController(duControllerPtr);

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
      duEventInterceptor.lastActorStateTransmitted(duPort),
      duActorStateBase | LacpState::IN_SYNC);
  // What the partner told us about itself should be reflected back
  ASSERT_EQ(
      duEventInterceptor.lastPartnerStateTransmitted(duPort), eswActorState);

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
      duEventInterceptor.lastActorStateTransmitted(duPort),
      duActorStateBase | LacpState::IN_SYNC | LacpState::COLLECTING |
          LacpState::DISTRIBUTING);
  // What the partner told us about iself should be reflected back
  ASSERT_EQ(
      duEventInterceptor.lastPartnerStateTransmitted(duPort),
      eswActorInfo.state);
  ASSERT(duEventInterceptor.isForwarding(duPort));

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
      duEventInterceptor.lastActorStateTransmitted(duPort),
      duActorStateBase | LacpState::IN_SYNC | LacpState::COLLECTING |
          LacpState::DISTRIBUTING);
  ASSERT_EQ(
      duEventInterceptor.lastPartnerStateTransmitted(duPort),
      eswActorInfo.state);
  // TODO(samank): really we should check that forwarding hasn't changed, even
  // twice
  ASSERT(duEventInterceptor.isForwarding(duPort));

  duControllerPtr->stopMachines();
}

TEST_F(LacpTest, UUColdBootReconvergenceWithDR) {
  LacpServiceInterceptor uuEventInterceptor(lacpEvb());

  constexpr std::size_t portCount = 3;

  std::array<ParticipantInfo, portCount> uuPortToParticipantInfo;
  for (std::size_t port = 0; port < portCount; ++port) {
    uuPortToParticipantInfo[port].systemPriority = 65535;
    uuPortToParticipantInfo[port].systemID = {
        {0x02, 0x90, 0xfb, 0x5e, 0x1e, 0x84}};
    uuPortToParticipantInfo[port].key = 21;
    uuPortToParticipantInfo[port].portPriority = 32768;
    uuPortToParticipantInfo[port].port = 4 * port + 118;
  }

  std::array<ParticipantInfo, portCount> drPortToParticipantInfo;
  for (std::size_t port = 0; port < portCount; ++port) {
    drPortToParticipantInfo[port].systemPriority = 127;
    drPortToParticipantInfo[port].systemID = {
        {0x84, 0xb5, 0x9c, 0xd6, 0x91, 0x44}};
    drPortToParticipantInfo[port].key = 23;
    drPortToParticipantInfo[port].portPriority = 127;
    drPortToParticipantInfo[port].port = port + 110;
  }

  std::array<PortID, portCount> uuPortIdxToPortID;
  for (std::size_t portIdx = 0; portIdx < portCount; ++portIdx) {
    uuPortIdxToPortID[portIdx] =
        static_cast<PortID>(uuPortToParticipantInfo[portIdx].port);
  }

  std::array<std::shared_ptr<LacpController>, portCount> controllerPtrs;
  for (std::size_t portIdx = 0; portIdx < portCount; ++portIdx) {
    auto info = uuPortToParticipantInfo[portIdx];

    controllerPtrs[portIdx] = std::make_shared<LacpController>(
        PortID(info.port),
        lacpEvb(),
        info.portPriority,
        cfg::LacpPortRate::FAST,
        cfg::LacpPortActivity::ACTIVE,
        AggregatePortID(info.key),
        info.systemPriority,
        MacAddress::fromBinary(
            folly::ByteRange(info.systemID.cbegin(), info.systemID.cend())),
        portCount /* minimum-link count */,
        &uuEventInterceptor);

    uuEventInterceptor.addController(controllerPtrs[portIdx]);
  }

  for (const auto& controllerPtr : controllerPtrs) {
    controllerPtr->startMachines();
  }

  for (const auto& controllerPtr : controllerPtrs) {
    controllerPtr->portUp();
  }

  // Some bits in LacpState are derived from configuration. To avoid having to
  // repreat them, they are factored out into the following variables
  const LacpState uuActorStateBase =
      LacpState::AGGREGATABLE | LacpState::ACTIVE | LacpState::SHORT_TIMEOUT;
  const LacpState drActorStateBase =
      LacpState::AGGREGATABLE | LacpState::ACTIVE | LacpState::SHORT_TIMEOUT;

  // We can't simply use {dr,uu}PortToParticipantInfo to fill in actor and
  // partner state in the first round-trip because the DR has yet to hear
  // from the UU, and so its partner information will be the DR's default
  // information
  ParticipantInfo initialActorInfo, initialPartnerInfo;
  for (std::size_t portIdx = 0; portIdx < portCount; ++portIdx) {
    initialActorInfo = drPortToParticipantInfo[portIdx];
    initialActorInfo.state =
        drActorStateBase | LacpState::DEFAULTED | LacpState::EXPIRED;

    // When an LACP speaker hasn't heard from its partner in a while, it
    // enters the DEFAULTED state and correspondingly uses defaulted information
    // for its partner.
    // IEEE 802.3AD and 802.1AX allow for this default information to be
    // configured by the administrator, but no implementation exposes this level
    // of control.
    // Implementations usually take the default partner information to be zero
    // for all fields. Junos is unique in that it does not take this approach.
    initialPartnerInfo.systemPriority = 1;
    initialPartnerInfo.systemID = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    // Junos guesses the partner is using the same key
    initialPartnerInfo.key = initialActorInfo.key;
    initialPartnerInfo.portPriority = 1;
    // Junos guesses the partner has the same port numbering
    initialPartnerInfo.port = initialActorInfo.port;
    // Junos guesses the partner has also timed out and is configured to be
    // aggregatable with a short timeout
    initialPartnerInfo.state = LacpState::DEFAULTED | LacpState::SHORT_TIMEOUT |
        LacpState::AGGREGATABLE;

    /* actorInfo=(SystemPriority 127,
     *            SystemID 84:b5:9c:d6:91:44,
     *            Key 23,
     *            PortPriority 127,
     *            Port {110,111,112},
     *            State 199)
     *
     * partnerInfo=(SystemPriority 1,
     *              SystemID 00:00:00:00:00:00,
     *              Key 23,
     *              PortPriority 1,
     *              Port {110,111,112},
     *              State 70)
     */
    controllerPtrs[portIdx]->received(
        LACPDU(initialActorInfo, initialPartnerInfo));

    // There is an obvious but easy to miss problem with structuring this code
    // as
    // 1. for each Controller c: c->received(...)
    // 2. for each Port p: ASSERT_EQ(P's last transmitted state, ...)
    // In the last iteration of (1), the satisfaction of min-links would result
    // in LacpState::IN_SYNC being set, which would cause us to miss the
    // transitional state tested below.
    if (portIdx != portCount - 1) {
      ASSERT_EQ(
          uuEventInterceptor.lastActorStateTransmitted(
              uuPortIdxToPortID[portIdx]),
          uuActorStateBase);
      ASSERT_EQ(
          uuEventInterceptor.lastPartnerStateTransmitted(
              uuPortIdxToPortID[portIdx]),
          initialActorInfo.state);
    }
  }

  // Each iteration of the above loop leaves the port corresponding to that
  // iteration in MuxMachine::WAITING. The very last iteration in the loop will
  // lead to the min-links contraint being satisfied.
  // At this point, every LacpController's MuxMachine transitions from
  // MuxState::WAITING to MuxState::ATTACHED. A mark of this internal transition
  // is the setting of the IN_SYNC bit in the next LACPDU transmitted.
  for (std::size_t portIdx = 0; portIdx < portCount; ++portIdx) {
    ASSERT_EQ(
        uuEventInterceptor.lastActorStateTransmitted(
            uuPortIdxToPortID[portIdx]),
        uuActorStateBase | LacpState::IN_SYNC);
    ASSERT_EQ(
        uuEventInterceptor.lastPartnerStateTransmitted(
            uuPortIdxToPortID[portIdx]),
        initialActorInfo.state);
  }

  // TODO(samank): The following transmission from the DR a
  for (std::size_t portIdx = 0; portIdx < portCount; ++portIdx) {
    if (portIdx == 2) {
      /* actorInfo=(SystemPriority 127,
       *            SystemID 84:b5:9c:d6:91:44,
       *            Key 23,
       *            PortPriority 127,
       *            Port 112,
       *            State 7)
       * partnerInfo=(SystemPriority 65535,
       *              SystemID 02:90:fb:5e:1e:84,
       *              Key 21,
       *              PortPriority 32768,
       *              Port 126,
       *              State 199)
       */
      drPortToParticipantInfo[portIdx].state = drActorStateBase;
      uuPortToParticipantInfo[portIdx].state =
          uuActorStateBase | LacpState::DEFAULTED | LacpState::EXPIRED;
      controllerPtrs[portIdx]->received(LACPDU(
          drPortToParticipantInfo[portIdx], uuPortToParticipantInfo[portIdx]));
    } else {
      /* actorInfo=(SystemPriority 127,
       *            SystemID 84:b5:9c:d6:91:44,
       *            Key 23,
       *            PortPriority 127,
       *            Port {110,111},
       *            State 7)
       * partnerInfo=(SystemPriority 65535,
       *              SystemID 02:90:fb:5e:1e:84,
       *              Key 21,
       *              PortPriority 32768,
       *              Port {118,122},
       *              State 7)
       */
      drPortToParticipantInfo[portIdx].state = drActorStateBase;
      uuPortToParticipantInfo[portIdx].state = uuActorStateBase;
      controllerPtrs[portIdx]->received(LACPDU(
          drPortToParticipantInfo[portIdx], uuPortToParticipantInfo[portIdx]));
    }
  }

  for (std::size_t portIdx = 0; portIdx < portCount; ++portIdx) {
    drPortToParticipantInfo[portIdx].state = drActorStateBase;
    uuPortToParticipantInfo[portIdx].state =
        uuActorStateBase | LacpState::IN_SYNC;
    /* actorInfo=(SystemPriority 127,
     *            SystemID 84:b5:9c:d6:91:44,
     *            Key 23,
     *            PortPriority 127,
     *            Port {110,111,112},
     *            State 7)
     * partnerInfo=(SystemPriority 65535,
     *              SystemID 02:90:fb:5e:1e:84,
     *              Key 21,
     *              PortPriority 32768,
     *              Port {118,122,126},
     *              State 15)
     */
    controllerPtrs[portIdx]->received(LACPDU(
        drPortToParticipantInfo[portIdx], uuPortToParticipantInfo[portIdx]));
  }

  for (std::size_t portIdx = 0; portIdx < portCount; ++portIdx) {
    drPortToParticipantInfo[portIdx].state = drActorStateBase |
        LacpState::COLLECTING | LacpState::DISTRIBUTING | LacpState::IN_SYNC;
    uuPortToParticipantInfo[portIdx].state =
        uuActorStateBase | LacpState::IN_SYNC;

    /* actorInfo=(SystemPriority 127,
     *            SystemID 84:b5:9c:d6:91:44,
     *            Key 23,
     *            PortPriority 127,
     *            Port {110,111,112},
     *            State 63)
     * partnerInfo=(SystemPriority 65535,
     *              SystemID 02:90:fb:5e:1e:84,
     *              Key 21,
     *              PortPriority 32768,
     *              Port {118,122,126},
     *              State 15)
     */
    controllerPtrs[portIdx]->received(LACPDU(
        drPortToParticipantInfo[portIdx], uuPortToParticipantInfo[portIdx]));

    // Having received the MATCHED signal in MuxState::ATTACHED, the MuxMachine
    // transitions to MuxState::COLLECTING_DISTRIBUTING.
    ASSERT(uuEventInterceptor.isForwarding(uuPortIdxToPortID[portIdx]));
  }

  std::this_thread::sleep_for(PeriodicTransmissionMachine::SHORT_PERIOD * 2);

  for (std::size_t portIdx = 0; portIdx < portCount; ++portIdx) {
    ASSERT_EQ(
        uuEventInterceptor.lastActorStateTransmitted(
            uuPortIdxToPortID[portIdx]),
        uuActorStateBase | LacpState::IN_SYNC | LacpState::COLLECTING |
            LacpState::DISTRIBUTING);
    ASSERT_EQ(
        uuEventInterceptor.lastPartnerStateTransmitted(
            uuPortIdxToPortID[portIdx]),
        drPortToParticipantInfo[portIdx].state);
  }

  for (const auto& controllerPtr : controllerPtrs) {
    controllerPtr->stopMachines();
  }
}
