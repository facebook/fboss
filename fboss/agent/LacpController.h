/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>

#include "fboss/agent/LacpMachines.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/types.h"

namespace folly {
class EventBase;
}

namespace facebook::fboss {

class AggregatePort;
class LacpServicerIf;

class LacpController : public std::enable_shared_from_this<LacpController> {
 public:
  LacpController(
      PortID portID,
      folly::EventBase* evb,
      LacpServicerIf* servicer);
  LacpController(
      PortID portID,
      folly::EventBase* evb,
      uint16_t portPriority,
      cfg::LacpPortRate rate,
      cfg::LacpPortActivity activity,
      uint16_t holdTimerMultiplier,
      AggregatePortID aggPortID,
      uint16_t systemPriority,
      folly::MacAddress systemID,
      uint8_t minLinkCount,
      LacpServicerIf* servicer);

  ~LacpController();

  void startMachines();
  void restoreMachines(const AggregatePort::PartnerState& partnerState);
  void stopMachines();

  // All of machines.cpp should execute in the context of the EventBase *evb()
  folly::EventBase* evb() const;

  // Invoked from LinkAggregationManager
  void portUp();
  void portDown();
  void received(const LACPDU& lacpdu);
  ParticipantInfo actorInfo() const;
  ParticipantInfo partnerInfo() const;

  // Invoked from TransmitMachine and *::updateState(*State) and
  // LinkAggregationGroupID::from(const Controller&)
  PortID portID() const;
  AggregatePortID aggregatePortID() const;

  // Inter-machine signals
  LacpState actorState() const;
  void setActorState(LacpState state);

  void ntt();
  void selected();
  void selected(folly::Range<std::vector<PortID>::const_iterator> ports);

  void unselected();
  void standby();
  void standby(folly::Range<std::vector<PortID>::const_iterator> ports);

  void matched(bool partnerChanged);
  void notMatched();

  void startPeriodicTransmissionMachine();
  void select();

 private:
  const AggregatePortID aggPortID_{0};
  const PortID portID_{0};
  const ParticipantInfo::PortPriority portPriority_{0};
  ParticipantInfo::SystemID systemID_{{0, 0, 0, 0, 0, 0}};
  const ParticipantInfo::SystemPriority systemPriority_{0};

  LacpState actorState_{LacpState::NONE};

  TransmitMachine tx_;
  ReceiveMachine rx_;
  PeriodicTransmissionMachine periodicTx_;
  MuxMachine mux_;
  Selector selector_;

  folly::EventBase* evb_{nullptr};
  LacpServicerIf* servicer_{nullptr};

  // Forbidden copy constructor and assignment operator
  LacpController(LacpController const&) = delete;
  LacpController& operator=(LacpController const&) = delete;
};

} // namespace facebook::fboss
