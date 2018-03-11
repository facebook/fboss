/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/LacpController.h"
#include "fboss/agent/LacpTypes-defs.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <folly/io/async/EventBase.h>

#include <cstring>

namespace facebook {
namespace fboss {

using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::string;

LacpController::LacpController(
    PortID portID,
    folly::EventBase* evb,
    LinkAggregationManager* lagMgr,
    SwSwitch* sw)
    : portID_(portID),
      tx_(*this, evb, sw),
      rx_(*this, evb),
      periodicTx_(*this, evb),
      mux_(*this, evb, sw),
      selector_(*this),
      evb_(evb),
      lagMgr_(lagMgr) {
  actorState_ &= ~LacpState::AGGREGATABLE;
}

LacpController::LacpController(
    PortID portID,
    folly::EventBase* evb,
    uint16_t portPriority,
    cfg::LacpPortRate rate,
    cfg::LacpPortActivity activity,
    AggregatePortID aggPortID,
    uint16_t systemPriority,
    folly::MacAddress systemID,
    uint8_t minLinkCount,
    LinkAggregationManager* lagMgr,
    SwSwitch* sw)
    : aggPortID_(aggPortID),
      portID_(portID),
      portPriority_(portPriority),
      systemPriority_(systemPriority),
      tx_(*this, evb, sw),
      rx_(*this, evb),
      periodicTx_(*this, evb),
      mux_(*this, evb, sw),
      selector_(*this, minLinkCount),
      evb_(evb),
      lagMgr_(lagMgr) {
  std::memcpy(systemID_.begin(), systemID.bytes(), systemID_.size());

  actorState_ |= LacpState::AGGREGATABLE;
  if (rate == cfg::LacpPortRate::FAST) {
    actorState_ |= LacpState::SHORT_TIMEOUT;
  }
  if (activity == cfg::LacpPortActivity::ACTIVE) {
    actorState_ |= LacpState::ACTIVE;
  }
}

void LacpController::startMachines() {
  evb()->runInEventBaseThread([self = shared_from_this()]() {
    self->mux_.start();
    self->tx_.start();
    self->periodicTx_.start();
    self->rx_.start();
    self->selector_.start();
  });
}

void LacpController::stopMachines() {
  evb()->runInEventBaseThread([self = shared_from_this()]() {
    self->mux_.stop();
    self->tx_.stop();
    self->periodicTx_.stop();
    self->rx_.stop();
    self->selector_.stop();
  });
}

LacpController::~LacpController() {}

folly::EventBase* LacpController::evb() const {
  return evb_;
}

void LacpController::portUp() {
  evb()->runInEventBaseThread([self = shared_from_this()]() {
    self->rx_.portUp();
    self->periodicTx_.portUp();
  });
}

void LacpController::portDown() {
  evb()->runInEventBaseThread([self = shared_from_this()]() {
    self->rx_.portDown();
    self->periodicTx_.portDown();
    self->selector_.portDown();
  });
}

void LacpController::receivedLACPDU(Cursor c) {
  auto lacpdu = LACPDU::from(&c);

  if (!lacpdu.isValid()) {
    return;
  }

  evb()->runInEventBaseThread(
      [self = shared_from_this(), lacpdu]() { self->rx_.rx(lacpdu); });
}

ParticipantInfo LacpController::actorInfo() const {
  ParticipantInfo info;

  evb()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [self = shared_from_this(), &info]() {
        info.systemID = self->systemID_;
        info.systemPriority = self->systemPriority_;
        info.port = static_cast<ParticipantInfo::Port>(self->portID_);
        info.portPriority = self->portPriority_;
        info.key = static_cast<ParticipantInfo::Key>(self->aggPortID_);
        info.state = self->actorState_;
      });

  return info;
}

void LacpController::setActorState(LacpState state) {
  actorState_ = state;
}

LacpState LacpController::actorState() const {
  return actorState_;
}

ParticipantInfo LacpController::partnerInfo() const {
  ParticipantInfo info;

  evb()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [self = shared_from_this(), &info]() { info = self->rx_.partnerInfo(); });

  return info;
}

void LacpController::ntt() {
  tx_.ntt(LACPDU(actorInfo(), partnerInfo()));
}

PortID LacpController::portID() const {
  return portID_;
}

AggregatePortID LacpController::aggregatePortID() const {
  return aggPortID_;
}

void LacpController::selected() {
  selector_.selected();
  auto sel = selector_.getSelection();
  mux_.selected(static_cast<AggregatePortID>(sel.lagID.actorKey));
}

void LacpController::unselected() {
  selector_.unselected();
  mux_.unselected();
}

void LacpController::standby() {
  mux_.standby();
  selector_.standby();
}

void LacpController::matched() {
  mux_.matched();
}

void LacpController::notMatched() {
  mux_.notMatched();
}

void LacpController::startPeriodicTransmissionMachine() {
  periodicTx_.start();
}

void LacpController::select() {
  selector_.select();
}

} // namespace fboss
} // namespace facebook
