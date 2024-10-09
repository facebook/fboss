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
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;

namespace {

class LacpTest : public ::testing::Test {
 protected:
  LacpTest() : lacpEvb_("LacpEventBase") {}

  void SetUp() override {
    lacpThread_.reset(new std::thread([this] {
      folly::setThreadName("testLACPthread");

      // TODO(samank): why doesn't this trip the CHECK...
      lacpEvb_.loopForever();
    }));
  }

  FbossEventBase* lacpEvb() {
    lacpEvb_.waitUntilRunning();
    return &lacpEvb_;
  }

  void TearDown() override {
    lacpEvb_.terminateLoopSoon();
    lacpThread_->join();
  }

 private:
  FbossEventBase lacpEvb_{"LacpEventBase"};
  std::unique_ptr<std::thread> lacpThread_{nullptr};
};

class LacpServiceInterceptor : public LacpServicerIf {
 public:
  explicit LacpServiceInterceptor(FbossEventBase* lacpEvb)
      : lacpEvb_(lacpEvb), sw_(nullptr) {}
  LacpServiceInterceptor(FbossEventBase* lacpEvb, SwSwitch* sw)
      : lacpEvb_(lacpEvb), sw_(sw) {}

  // The following methods implement the LacpServicerIf interface
  bool transmit(LACPDU lacpdu, PortID portID) override {
    auto portToLastTransmissionLocked = portToLastTransmission_.wlock();

    (*portToLastTransmissionLocked)[portID] = lacpdu;

    // "Transmit" the frame
    return true;
  }
  void enableForwardingAndSetPartnerState(
      PortID portID,
      AggregatePortID aggPortID,
      const ParticipantInfo& /* unused */) override {
    auto portToIsForwardingLocked = portToIsForwarding_.wlock();

    (*portToIsForwardingLocked)[portID] = true;

    // "Enable" forwarding
    XLOG(DBG2) << "Enabling member " << portID << " in " << "aggregate "
               << aggPortID;
  }
  void disableForwardingAndSetPartnerState(
      PortID portID,
      AggregatePortID aggPortID,
      const ParticipantInfo& /* unused */) override {
    auto portToIsForwardingLocked = portToIsForwarding_.wlock();

    (*portToIsForwardingLocked)[portID] = false;

    // "Disable" forwarding
    XLOG(DBG2) << "Disabling member " << portID << " in " << "aggregate "
               << aggPortID;
  }

  void recordLacpTimeout() override {
    if (sw_) {
      sw_->stats()->LacpRxTimeouts();
    }
  }
  void recordLacpMismatchPduTeardown() override {
    if (sw_) {
      sw_->stats()->LacpMismatchPduTeardown();
    }
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

    lacpEvb_->runInFbossEventBaseThreadAndWait(
        [this, portID, &actorStateTransmitted]() {
          auto lastTransmission = portToLastTransmission_.rlock()->at(portID);
          actorStateTransmitted = lastTransmission.actorInfo.state;
        });

    return actorStateTransmitted;
  }
  LacpState lastPartnerStateTransmitted(PortID portID) {
    LacpState partnerStateTransmitted = LacpState::NONE;

    lacpEvb_->runInFbossEventBaseThreadAndWait(
        [this, portID, &partnerStateTransmitted]() {
          auto lastTransmission = portToLastTransmission_.rlock()->at(portID);
          partnerStateTransmitted = lastTransmission.partnerInfo.state;
        });

    return partnerStateTransmitted;
  }
  LACPDU lastLacpduTransmitted(PortID portID) {
    LACPDU lacpduTransmitted;

    lacpEvb_->runInFbossEventBaseThreadAndWait(
        [this, portID, &lacpduTransmitted]() {
          lacpduTransmitted = portToLastTransmission_.rlock()->at(portID);
        });

    return lacpduTransmitted;
  }
  bool isForwarding(PortID portID) {
    bool forwarding = false;

    lacpEvb_->runInFbossEventBaseThreadAndWait([this, portID, &forwarding]() {
      forwarding = portToIsForwarding_.rlock()->at(portID);
    });

    return forwarding;
  }

  ~LacpServiceInterceptor() override {
    lacpEvb_->runInFbossEventBaseThreadAndWait([this]() {
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

  FbossEventBase* lacpEvb_{nullptr};
  SwSwitch* sw_{nullptr};
};

class MockLacpServicer : public LacpServicerIf {
  void enableForwardingAndSetPartnerState(
      PortID portID,
      AggregatePortID aggPortID,
      const ParticipantInfo& /* unused */) override {
    XLOG(DBG2) << "Enabling member " << portID << " in " << "aggregate "
               << aggPortID;
  }
  void disableForwardingAndSetPartnerState(
      PortID portID,
      AggregatePortID aggPortID,
      const ParticipantInfo& /* unused */) override {
    XLOG(DBG2) << "Disabling member " << portID << " in " << "aggregate "
               << aggPortID;
  }
  void recordLacpTimeout() override {}
  void recordLacpMismatchPduTeardown() override {}
  MOCK_METHOD2(transmit, bool(LACPDU, PortID));
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
  // PortID, LacpServiceIf*, and FbossEventBase*.

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
  actorInfo.state = LacpState::LACP_ACTIVE | LacpState::AGGREGATABLE;
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

void DUColdBootReconvergenceWithESWHelper(
    FbossEventBase* lacpEvb,
    cfg::LacpPortRate rate) {
  LacpServiceInterceptor duEventInterceptor(lacpEvb);

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
  LacpState eswActorStateBase =
      LacpState::AGGREGATABLE | LacpState::LACP_ACTIVE;
  LacpState duActorStateBase = LacpState::AGGREGATABLE | LacpState::LACP_ACTIVE;

  if (rate == cfg::LacpPortRate::FAST) {
    eswActorStateBase |= LacpState::SHORT_TIMEOUT;
    duActorStateBase |= LacpState::SHORT_TIMEOUT;
  }

  // duController contains the totality of the LACP logic we would like to test
  // Although its lifetime coincides with this stack frame, duController's
  // methods assume it is being managed by a shared_ptr
  auto duControllerPtr = std::make_shared<LacpController>(
      PortID(duInfo.port),
      lacpEvb,
      duInfo.portPriority,
      rate,
      cfg::LacpPortActivity::ACTIVE,
      cfg::switch_config_constants::DEFAULT_LACP_HOLD_TIMER_MULTIPLIER(),
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
  eswPartnerState = (rate == cfg::LacpPortRate::SLOW)
      ? LacpState::NONE
      : LacpState::SHORT_TIMEOUT;

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
   *            State 13/15 (slow/fast))
   */
  eswActorInfo = eswInfo;
  eswActorInfo.state = eswActorState;

  /*
   * partnerInfo=(SystemPriority 65535,
   *              SystemID 02:90:fb:5e:1e:8d,
   *              Key 903,
   *              PortPriority 32768,
   *              Port 42,
   *              State 13/15 (slow/fast))
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
  ASSERT_TRUE(duEventInterceptor.isForwarding(duPort));

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
   *            State 61/63 (slow/fast))
   */
  eswActorInfo = eswInfo;
  eswActorInfo.state = eswActorState;

  /*
   * partnerInfo=(SystemPriority 65535,
   *              SystemID 02:90:fb:5e:1e:8d,
   *              Key 903,
   *              PortPriority 32768,
   *              Port 42,
   *              State 61/63(slow/fast))
   */
  eswPartnerInfo = duInfo;
  eswPartnerInfo.state = eswPartnerState;

  duControllerPtr->received(LACPDU(eswActorInfo, eswPartnerInfo));

  // TODO(samank): how would a PASSIVE LACP speaker handle this?
  // Waiting double the PeriodicTransmissionMachine's tranmission period
  // guarantees a new LACP frame has been transmitted
  auto period = (rate == cfg::LacpPortRate::SLOW)
      ? PeriodicTransmissionMachine::LONG_PERIOD * 2
      : PeriodicTransmissionMachine::SHORT_PERIOD * 2;
  std::this_thread::sleep_for(period);

  ASSERT_EQ(
      duEventInterceptor.lastActorStateTransmitted(duPort),
      duActorStateBase | LacpState::IN_SYNC | LacpState::COLLECTING |
          LacpState::DISTRIBUTING);
  ASSERT_EQ(
      duEventInterceptor.lastPartnerStateTransmitted(duPort),
      eswActorInfo.state);
  // TODO(samank): really we should check that forwarding hasn't changed, even
  // twice
  ASSERT_TRUE(duEventInterceptor.isForwarding(duPort));

  duControllerPtr->stopMachines();
}

TEST_F(LacpTest, DUColdBootReconvergenceWithESWLacpSlow) {
  DUColdBootReconvergenceWithESWHelper(lacpEvb(), cfg::LacpPortRate::SLOW);
}

TEST_F(LacpTest, DUColdBootReconvergenceWithESWLacpFast) {
  DUColdBootReconvergenceWithESWHelper(lacpEvb(), cfg::LacpPortRate::FAST);
}

void UUColdBootReconvergenceWithDRHelper(
    FbossEventBase* lacpEvb,
    cfg::LacpPortRate rate) {
  LacpServiceInterceptor uuEventInterceptor(lacpEvb);

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
        lacpEvb,
        info.portPriority,
        rate,
        cfg::LacpPortActivity::ACTIVE,
        cfg::switch_config_constants::DEFAULT_LACP_HOLD_TIMER_MULTIPLIER(),
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
  LacpState uuActorStateBase = LacpState::AGGREGATABLE | LacpState::LACP_ACTIVE;
  LacpState drActorStateBase = LacpState::AGGREGATABLE | LacpState::LACP_ACTIVE;
  if (rate == cfg::LacpPortRate::FAST) {
    uuActorStateBase |= LacpState::SHORT_TIMEOUT;
    drActorStateBase |= LacpState::SHORT_TIMEOUT;
  }

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
    initialPartnerInfo.state = LacpState::DEFAULTED | LacpState::AGGREGATABLE;
    if (rate == cfg::LacpPortRate::FAST) {
      initialPartnerInfo.state |= LacpState::SHORT_TIMEOUT;
    }

    /* actorInfo=(SystemPriority 127,
     *            SystemID 84:b5:9c:d6:91:44,
     *            Key 23,
     *            PortPriority 127,
     *            Port {110,111,112},
     *            State 197/199 (slow/fast))
     *
     * partnerInfo=(SystemPriority 1,
     *              SystemID 00:00:00:00:00:00,
     *              Key 23,
     *              PortPriority 1,
     *              Port {110,111,112},
     *              State 68/70 (slow/fast))
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
       *            State 5/7 (slow/fast))
       * partnerInfo=(SystemPriority 65535,
       *              SystemID 02:90:fb:5e:1e:84,
       *              Key 21,
       *              PortPriority 32768,
       *              Port 126,
       *              State 197/199 (slow/fast))
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
       *            State 5/7 (slow/fast))
       * partnerInfo=(SystemPriority 65535,
       *              SystemID 02:90:fb:5e:1e:84,
       *              Key 21,
       *              PortPriority 32768,
       *              Port {118,122},
       *              State 5/7 (slow/fast))
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
     *            State 5/7 (slow/fast))
     * partnerInfo=(SystemPriority 65535,
     *              SystemID 02:90:fb:5e:1e:84,
     *              Key 21,
     *              PortPriority 32768,
     *              Port {118,122,126},
     *              State 13/15 (slow/fast))
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
     *            State 61)
     * partnerInfo=(SystemPriority 65535,
     *              SystemID 02:90:fb:5e:1e:84,
     *              Key 21,
     *              PortPriority 32768,
     *              Port {118,122,126},
     *              State 13)
     */
    controllerPtrs[portIdx]->received(LACPDU(
        drPortToParticipantInfo[portIdx], uuPortToParticipantInfo[portIdx]));

    // Having received the MATCHED signal in MuxState::ATTACHED, the MuxMachine
    // transitions to MuxState::COLLECTING_DISTRIBUTING.
    ASSERT_TRUE(uuEventInterceptor.isForwarding(uuPortIdxToPortID[portIdx]));
  }
  auto period = (rate == cfg::LacpPortRate::SLOW)
      ? PeriodicTransmissionMachine::LONG_PERIOD * 2
      : PeriodicTransmissionMachine::SHORT_PERIOD * 2;
  std::this_thread::sleep_for(period);

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

TEST_F(LacpTest, UUColdBootReconvergenceWithDRLacpSlow) {
  UUColdBootReconvergenceWithDRHelper(lacpEvb(), cfg::LacpPortRate::SLOW);
}

TEST_F(LacpTest, UUColdBootReconvergenceWithDRLacpFast) {
  UUColdBootReconvergenceWithDRHelper(lacpEvb(), cfg::LacpPortRate::FAST);
}

void selfInteroperabilityHelper(
    FbossEventBase* lacpEvb,
    cfg::LacpPortRate rate,
    uint16_t holdTimer) {
  LacpServiceInterceptor uuEventInterceptor(lacpEvb);
  LacpServiceInterceptor duEventInterceptor(lacpEvb);

  PortID uuPort(static_cast<uint16_t>(0xA));
  PortID duPort(static_cast<uint16_t>(0xD));

  ParticipantInfo uuInfo;
  uuInfo.systemPriority = 65535;
  uuInfo.systemID = {{0x02, 0x90, 0xfb, 0x5e, 0x1e, 0x85}};
  uuInfo.key = uuPort;
  uuInfo.portPriority = 32768;
  uuInfo.port = uuPort;
  uuInfo.state = LacpState::NONE;
  ParticipantInfo duInfo;
  duInfo.systemPriority = 65535;
  duInfo.systemID = {{0x02, 0x90, 0xfb, 0x5e, 0x24, 0x28}};
  duInfo.key = duPort;
  duInfo.portPriority = 32768;
  duInfo.port = duPort;
  duInfo.state = LacpState::NONE;

  auto uuControllerPtr = std::make_shared<LacpController>(
      uuPort,
      lacpEvb,
      uuInfo.portPriority,
      rate,
      cfg::LacpPortActivity::ACTIVE,
      holdTimer,
      AggregatePortID(uuInfo.key),
      uuInfo.systemPriority,
      MacAddress::fromBinary(
          folly::ByteRange(uuInfo.systemID.cbegin(), uuInfo.systemID.cend())),
      1 /* minimum-link count */,
      &uuEventInterceptor);
  uuEventInterceptor.addController(uuControllerPtr);
  auto duControllerPtr = std::make_shared<LacpController>(
      duPort,
      lacpEvb,
      duInfo.portPriority,
      rate,
      cfg::LacpPortActivity::ACTIVE,
      holdTimer,
      AggregatePortID(duInfo.key),
      duInfo.systemPriority,
      MacAddress::fromBinary(
          folly::ByteRange(duInfo.systemID.cbegin(), duInfo.systemID.cend())),
      1 /* minimum-link count */,
      &duEventInterceptor);
  duEventInterceptor.addController(duControllerPtr);

  LACPDU uuTransmission;
  LACPDU duTransmission;

  uuControllerPtr->startMachines();
  uuControllerPtr->portUp();

  auto period = (rate == cfg::LacpPortRate::SLOW)
      ? PeriodicTransmissionMachine::LONG_PERIOD * 2
      : PeriodicTransmissionMachine::SHORT_PERIOD * 2;
  std::this_thread::sleep_for(period);
  /*
   *   actorInfo=(SystemPriority 65535,
   *              SystemID 02:90:fb:5e:1e:85,
   *              Key 10,
   *              PortPriority 32768,
   *              Port 10,
   *              State ACTIVE | SHORT_TIMEOUT(fast) |AGGREGATABLE | DEFAULTED
   *                    EXPIRED)
   * partnerInfo=(SystemPriority 0,
   *              SystemID 0,
   *              Key 0,
   *              PortPriority 0,
   *              Port 0,
   *              State SHORT_TIMEOUT(fast))
   */
  uuTransmission = uuEventInterceptor.lastLacpduTransmitted(uuPort);

  // By starting the controller here, it's as if the du was cold-booted
  // while the uu was up
  duControllerPtr->startMachines();
  duControllerPtr->portUp();

  duControllerPtr->received(uuTransmission);
  /*
   * actorInfo=(SystemPriority 65535,
   *            SystemID 02:90:fb:5e:24:28,
   *            Key 13,
   *            PortPriority 32768,
   *            Port 13,
   *            State ACTIVE | SHORT_TIMEOUT(fast) | AGGREGATABLE | EXPIRED)
   * partnerInfo=(SystemPriority 65535,
   *              SystemID 02:90:fb:5e:1e:85,
   *              Key 10,
   *              PortPriority 32768,
   *              Port 10,
   *              State ACTIVE | SHORT_TIMEOUT(fast) | AGGREGATABLE | DEFAULTED
   *                    EXPIRED)
   */
  duTransmission = duEventInterceptor.lastLacpduTransmitted(duPort);

  uuControllerPtr->received(duTransmission);
  /*
   * actorInfo=(SystemPriority 65535,
   *            SystemID 02:90:fb:5e:1e:85,
   *            Key 10,
   *            PortPriority 32768,
   *            Port 10,
   *            State ACTIVE | SHORT_TIMEOUT(fast) | AGGREGATABLE | IN_SYNC
                    | COLLECTING | DISTRIBUTING)
   * parnterInfo=(SystemPriority 65535,
   *            SystemID 02:90:fb:5e:24:28,
   *            Key 13,
   *            PortPriority 32768,
   *            Port 13,
   *            State ACTIVE | SHORT_TIMEOUT(fast) | AGGREGATABLE)
   */
  uuTransmission = uuEventInterceptor.lastLacpduTransmitted(uuPort);

  duControllerPtr->received(uuTransmission);
  /*
   * actorInfo=(SystemPriority 65535,
   *            SystemID 02:90:fb:5e:24:28,
   *            Key 13,
   *            PortPriority 32768,
   *            Port 13,
   *            State ACTIVE | SHORT_TIMEOUT(fast) | AGGREGATABLE | IN_SYNC
   *                | COLLECTING | DISTRIBUTING)
   * partnerInfo=(SystemPriority 65535,
   *            SystemID 02:90:fb:5e:1e:85,
   *            Key 10,
   *            PortPriority 32768,
   *            Port 10,
   *            State ACTIVE | SHORT_TIMEOUT(fast) | AGGREGATABLE | IN_SYNC
   *            | COLLECTING | DISTRIBUTING)
   */
  duTransmission = duEventInterceptor.lastLacpduTransmitted(duPort);

  // TODO(samank): check this is a no-op on the UU endpoint
  uuControllerPtr->received(duTransmission);

  auto state = LacpState::AGGREGATABLE | LacpState::LACP_ACTIVE |
      LacpState::IN_SYNC | LacpState::COLLECTING | LacpState::DISTRIBUTING;
  if (rate == cfg::LacpPortRate::FAST) {
    state |= LacpState::SHORT_TIMEOUT;
  }
  ASSERT_EQ(duEventInterceptor.lastActorStateTransmitted(duPort), state);
  ASSERT_EQ(uuEventInterceptor.lastActorStateTransmitted(uuPort), state);

  // verify that Rx waits till hold timer value before expiring
  period = (rate == cfg::LacpPortRate::SLOW)
      ? PeriodicTransmissionMachine::LONG_PERIOD * (holdTimer - 1)
      : PeriodicTransmissionMachine::SHORT_PERIOD * (holdTimer - 1);
  std::this_thread::sleep_for(period);
  ASSERT_EQ(duEventInterceptor.lastActorStateTransmitted(duPort), state);
  ASSERT_EQ(uuEventInterceptor.lastActorStateTransmitted(uuPort), state);

  // cross the boundary for wait time and now rx should expire
  period = (rate == cfg::LacpPortRate::SLOW)
      ? PeriodicTransmissionMachine::LONG_PERIOD * 2
      : PeriodicTransmissionMachine::SHORT_PERIOD * 3;
  std::this_thread::sleep_for(period);
  ASSERT_NE(duEventInterceptor.lastActorStateTransmitted(duPort), state);
  ASSERT_NE(uuEventInterceptor.lastActorStateTransmitted(uuPort), state);
}

TEST_F(LacpTest, selfInteroperabilityLacpSlow) {
  selfInteroperabilityHelper(
      lacpEvb(),
      cfg::LacpPortRate::SLOW,
      cfg::switch_config_constants::DEFAULT_LACP_HOLD_TIMER_MULTIPLIER());
}

TEST_F(LacpTest, selfInteroperabilityLacpFast) {
  selfInteroperabilityHelper(
      lacpEvb(),
      cfg::LacpPortRate::FAST,
      cfg::switch_config_constants::DEFAULT_LACP_HOLD_TIMER_MULTIPLIER());
}

TEST_F(LacpTest, selfInteroperabilityLacpSlowCustomTimer) {
  selfInteroperabilityHelper(lacpEvb(), cfg::LacpPortRate::SLOW, 5);
}

TEST_F(LacpTest, selfInteroperabilityLacpFastCustomTimer) {
  selfInteroperabilityHelper(lacpEvb(), cfg::LacpPortRate::FAST, 5);
}

/*
 * Restore state machines after warmboot and verify that
 * Rx/Tx continues with saved state
 */
void selfInteroperabilityAfterWarmbootHelper(
    FbossEventBase* lacpEvb,
    cfg::LacpPortRate rate) {
  LacpServiceInterceptor uuEventInterceptor(lacpEvb);
  LacpServiceInterceptor duEventInterceptor(lacpEvb);

  PortID uuPort(static_cast<uint16_t>(0xA));
  PortID duPort(static_cast<uint16_t>(0xD));

  using SystemID = std::array<uint8_t, 6>;
  auto makeParticipantInfo = [&rate](SystemID systemID, auto port) {
    ParticipantInfo pInfo;
    pInfo.systemPriority = 65535;
    pInfo.systemID = systemID;
    pInfo.key = port;
    pInfo.portPriority = 32768;
    pInfo.port = port;
    pInfo.state = LacpState::LACP_ACTIVE | LacpState::AGGREGATABLE |
        LacpState::COLLECTING | LacpState::DISTRIBUTING | LacpState::IN_SYNC;
    if (rate == cfg::LacpPortRate::FAST) {
      pInfo.state |= LacpState::SHORT_TIMEOUT;
    }
    return pInfo;
  };

  ParticipantInfo uuInfo =
      makeParticipantInfo({0x02, 0x90, 0xfb, 0x5e, 0x1e, 0x85}, uuPort);
  ParticipantInfo duInfo =
      makeParticipantInfo({0x02, 0x90, 0xfb, 0x5e, 0x24, 0x28}, duPort);

  auto uuController = std::make_shared<LacpController>(
      uuPort,
      lacpEvb,
      uuInfo.portPriority,
      rate,
      cfg::LacpPortActivity::ACTIVE,
      cfg::switch_config_constants::DEFAULT_LACP_HOLD_TIMER_MULTIPLIER(),
      AggregatePortID(uuInfo.key),
      uuInfo.systemPriority,
      MacAddress::fromBinary(
          folly::ByteRange(uuInfo.systemID.cbegin(), uuInfo.systemID.cend())),
      1 /* minimum-link count */,
      &uuEventInterceptor);
  uuEventInterceptor.addController(uuController);
  auto duController = std::make_shared<LacpController>(
      duPort,
      lacpEvb,
      duInfo.portPriority,
      rate,
      cfg::LacpPortActivity::ACTIVE,
      cfg::switch_config_constants::DEFAULT_LACP_HOLD_TIMER_MULTIPLIER(),
      AggregatePortID(duInfo.key),
      duInfo.systemPriority,
      MacAddress::fromBinary(
          folly::ByteRange(duInfo.systemID.cbegin(), duInfo.systemID.cend())),
      1 /* minimum-link count */,
      &duEventInterceptor);
  duEventInterceptor.addController(duController);

  AggregatePort::PartnerState uuPartnerState =
      makeParticipantInfo(duInfo.systemID, duInfo.port);
  uuController->restoreMachines(uuPartnerState);

  AggregatePort::PartnerState duPartnerState =
      makeParticipantInfo(uuInfo.systemID, uuInfo.port);
  duController->restoreMachines(duPartnerState);
  XLOG(DBG3) << "Restored LACP state machines";

  LACPDU uuTransmission;
  LACPDU duTransmission;
  uuTransmission = uuEventInterceptor.lastLacpduTransmitted(uuPort);
  duController->received(uuTransmission);
  duTransmission = duEventInterceptor.lastLacpduTransmitted(duPort);
  uuController->received(duTransmission);

  auto state = LacpState::AGGREGATABLE | LacpState::LACP_ACTIVE |
      LacpState::IN_SYNC | LacpState::COLLECTING | LacpState::DISTRIBUTING;
  if (rate == cfg::LacpPortRate::FAST) {
    state |= LacpState::SHORT_TIMEOUT;
  }
  ASSERT_EQ(duEventInterceptor.lastActorStateTransmitted(duPort), state);
  ASSERT_EQ(duEventInterceptor.lastPartnerStateTransmitted(duPort), state);
  ASSERT_EQ(uuEventInterceptor.lastActorStateTransmitted(uuPort), state);
  ASSERT_EQ(uuEventInterceptor.lastPartnerStateTransmitted(uuPort), state);
}

TEST_F(LacpTest, selfInteroperabilityAfterWarmbootSlow) {
  selfInteroperabilityAfterWarmbootHelper(lacpEvb(), cfg::LacpPortRate::SLOW);
}

TEST_F(LacpTest, selfInteroperabilityAfterWarmbootFast) {
  selfInteroperabilityAfterWarmbootHelper(lacpEvb(), cfg::LacpPortRate::FAST);
}

/*
 * LAG with 2 member links between DU and UU. When one of DU port goes
 * down, both member LACP sessions go down to min link config.
 * The second DU port is physically UP but in detached state.
 * A PDU is received on UP port but the port should not transition to
 * fwding enabled state
 */
TEST_F(LacpTest, lacpPortFlapAfterSync) {
  auto rate = cfg::LacpPortRate::SLOW;
  FbossEventBase* lacpEvbase = lacpEvb();
  LacpServiceInterceptor uuEventInterceptor(lacpEvbase);
  LacpServiceInterceptor duEventInterceptor(lacpEvbase);

  PortID uuPort1(static_cast<uint16_t>(0xA));
  PortID duPort1(static_cast<uint16_t>(0xD));
  PortID uuPort2(static_cast<uint16_t>(0xB));
  PortID duPort2(static_cast<uint16_t>(0xE));

  using SystemID = std::array<uint8_t, 6>;
  auto makeParticipantInfo = [&rate](SystemID systemID, auto port, auto key) {
    ParticipantInfo pInfo;
    pInfo.systemPriority = 65535;
    pInfo.systemID = systemID;
    pInfo.key = key;
    pInfo.portPriority = 32768;
    pInfo.port = port;
    pInfo.state = LacpState::LACP_ACTIVE | LacpState::AGGREGATABLE |
        LacpState::COLLECTING | LacpState::DISTRIBUTING | LacpState::IN_SYNC;
    if (rate == cfg::LacpPortRate::FAST) {
      pInfo.state |= LacpState::SHORT_TIMEOUT;
    }
    return pInfo;
  };

  ParticipantInfo uuInfo1 =
      makeParticipantInfo({0x02, 0x90, 0xfb, 0x5e, 0x1e, 0x85}, uuPort1, 1);
  ParticipantInfo uuInfo2 =
      makeParticipantInfo({0x02, 0x90, 0xfb, 0x5e, 0x1e, 0x85}, uuPort2, 1);
  ParticipantInfo duInfo1 =
      makeParticipantInfo({0x02, 0x90, 0xfb, 0x5e, 0x24, 0x28}, duPort1, 2);
  ParticipantInfo duInfo2 =
      makeParticipantInfo({0x02, 0x90, 0xfb, 0x5e, 0x24, 0x28}, duPort2, 2);

  auto createAndAddController =
      [&rate, &lacpEvbase](auto& port, auto& pInfo, auto& interceptor) {
        auto controller = std::make_shared<LacpController>(
            port,
            lacpEvbase,
            pInfo.portPriority,
            rate,
            cfg::LacpPortActivity::ACTIVE,
            cfg::switch_config_constants::DEFAULT_LACP_HOLD_TIMER_MULTIPLIER(),
            AggregatePortID(pInfo.key),
            pInfo.systemPriority,
            MacAddress::fromBinary(folly::ByteRange(
                pInfo.systemID.cbegin(), pInfo.systemID.cend())),
            2 /* minimum-link count */,
            &interceptor);
        interceptor.addController(controller);
        return controller;
      };

  auto uuController1 =
      createAndAddController(uuPort1, uuInfo1, uuEventInterceptor);
  auto uuController2 =
      createAndAddController(uuPort2, uuInfo2, uuEventInterceptor);
  auto duController1 =
      createAndAddController(duPort1, duInfo1, duEventInterceptor);
  auto duController2 =
      createAndAddController(duPort2, duInfo2, duEventInterceptor);

  AggregatePort::PartnerState partnerState =
      makeParticipantInfo(duInfo1.systemID, duInfo1.port, 2);
  uuController1->restoreMachines(partnerState);
  partnerState = makeParticipantInfo(uuInfo1.systemID, uuInfo1.port, 1);
  duController1->restoreMachines(partnerState);
  partnerState = makeParticipantInfo(duInfo2.systemID, duInfo2.port, 2);
  uuController2->restoreMachines(partnerState);
  partnerState = makeParticipantInfo(uuInfo2.systemID, uuInfo2.port, 1);
  duController2->restoreMachines(partnerState);
  XLOG(DBG3) << "Restored LACP state machines";

  // All ports should be in forwarding state
  ASSERT_EQ(duEventInterceptor.isForwarding(duPort1), true);
  ASSERT_EQ(duEventInterceptor.isForwarding(duPort2), true);
  ASSERT_EQ(uuEventInterceptor.isForwarding(uuPort1), true);
  ASSERT_EQ(uuEventInterceptor.isForwarding(uuPort2), true);

  auto uuTransmission = uuEventInterceptor.lastLacpduTransmitted(uuPort2);
  // One UU port transitions to down
  duController1->portDown();
  // Both UU ports should transition to fwd disabled state due to min link
  // configuration.
  ASSERT_EQ(duEventInterceptor.isForwarding(duPort1), false);
  ASSERT_EQ(duEventInterceptor.isForwarding(duPort2), false);

  uuTransmission.actorInfo.state =
      LacpState::LACP_ACTIVE | LacpState::AGGREGATABLE | LacpState::IN_SYNC;
  // A PDU is received on UP port
  duController2->received(uuTransmission);
  // the port should stay down
  ASSERT_EQ(duEventInterceptor.isForwarding(duPort2), false);
}
/*
 * Test LACP counters for timeout and PDU teardown
 */
TEST_F(LacpTest, lacpDownCounters) {
  auto rate = cfg::LacpPortRate::FAST;
  auto lacpEvbase = lacpEvb();
  cfg::SwitchConfig config;
  auto handle = createTestHandle(&config);
  auto sw = handle->getSw();
  LacpServiceInterceptor uuEventInterceptor(lacpEvbase);
  // Start DU (which is the unit under test) with SwSwitch.
  // Only one swswitch can run at a time. So UU runs without it.
  LacpServiceInterceptor duEventInterceptor(lacpEvbase, sw);

  PortID uuPort1(static_cast<uint16_t>(0xA));
  PortID duPort1(static_cast<uint16_t>(0xD));
  PortID uuPort2(static_cast<uint16_t>(0xB));
  PortID duPort2(static_cast<uint16_t>(0xE));

  using SystemID = std::array<uint8_t, 6>;
  auto makeParticipantInfo = [](SystemID systemID, auto port, auto key) {
    ParticipantInfo pInfo;
    pInfo.systemPriority = 65535;
    pInfo.systemID = systemID;
    pInfo.key = key;
    pInfo.portPriority = 32768;
    pInfo.port = port;
    pInfo.state = LacpState::LACP_ACTIVE | LacpState::AGGREGATABLE |
        LacpState::COLLECTING | LacpState::DISTRIBUTING | LacpState::IN_SYNC |
        LacpState::SHORT_TIMEOUT;
    return pInfo;
  };

  SystemID uuSystemId = {0x02, 0x90, 0xfb, 0x5e, 0x1e, 0x85};
  ParticipantInfo uuInfo1 = makeParticipantInfo(uuSystemId, uuPort1, 1);
  ParticipantInfo uuInfo2 = makeParticipantInfo(uuSystemId, uuPort2, 1);
  SystemID duSystemId = {0x02, 0x90, 0xfb, 0x5e, 0x24, 0x28};
  ParticipantInfo duInfo1 = makeParticipantInfo(duSystemId, duPort1, 2);
  ParticipantInfo duInfo2 = makeParticipantInfo(duSystemId, duPort2, 2);

  auto createAndAddController =
      [&rate, &lacpEvbase](auto& port, auto& pInfo, auto& interceptor) {
        auto controller = std::make_shared<LacpController>(
            port,
            lacpEvbase,
            pInfo.portPriority,
            rate,
            cfg::LacpPortActivity::ACTIVE,
            cfg::switch_config_constants::DEFAULT_LACP_HOLD_TIMER_MULTIPLIER(),
            AggregatePortID(pInfo.key),
            pInfo.systemPriority,
            MacAddress::fromBinary(folly::ByteRange(
                pInfo.systemID.cbegin(), pInfo.systemID.cend())),
            2 /* minimum-link count */,
            &interceptor);
        interceptor.addController(controller);
        return controller;
      };

  auto uuController1 =
      createAndAddController(uuPort1, uuInfo1, uuEventInterceptor);
  auto uuController2 =
      createAndAddController(uuPort2, uuInfo2, uuEventInterceptor);
  auto duController1 =
      createAndAddController(duPort1, duInfo1, duEventInterceptor);
  auto duController2 =
      createAndAddController(duPort2, duInfo2, duEventInterceptor);

  auto partnerStateUu1 = makeParticipantInfo(duInfo1.systemID, duInfo1.port, 2);
  uuController1->restoreMachines(partnerStateUu1);
  auto partnerStateDu1 = makeParticipantInfo(uuInfo1.systemID, uuInfo1.port, 1);
  duController1->restoreMachines(partnerStateDu1);
  auto partnerStateUu2 = makeParticipantInfo(duInfo2.systemID, duInfo2.port, 2);
  uuController2->restoreMachines(partnerStateUu2);
  auto partnerStateDu2 = makeParticipantInfo(uuInfo2.systemID, uuInfo2.port, 1);
  duController2->restoreMachines(partnerStateDu2);
  XLOG(DBG3) << "Restored LACP state machines";

  // All ports should be in forwarding state
  ASSERT_EQ(duEventInterceptor.isForwarding(duPort1), true);
  ASSERT_EQ(duEventInterceptor.isForwarding(duPort2), true);
  ASSERT_EQ(uuEventInterceptor.isForwarding(uuPort1), true);
  ASSERT_EQ(uuEventInterceptor.isForwarding(uuPort2), true);

  auto uuTransmission = uuEventInterceptor.lastLacpduTransmitted(uuPort2);
  uuTransmission.actorInfo.state =
      LacpState::LACP_ACTIVE | LacpState::AGGREGATABLE;
  // Cache the current stats
  CounterCache counters(sw);
  // A mismatched PDU is received on port which transitions it to down
  duController2->received(uuTransmission);
  auto period = PeriodicTransmissionMachine::SHORT_PERIOD * 4;
  // wait for LACP to timeout
  std::this_thread::sleep_for(period);
  counters.update();
  // one member should record mismatch pdu
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "lacp.mismatched_pdu_teardown.sum", 1);
  // both members should timeout
  counters.checkDelta(SwitchStats::kCounterPrefix + "lacp.rx_timeout.sum", 2);
}
