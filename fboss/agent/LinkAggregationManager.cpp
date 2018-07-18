/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/LinkAggregationManager.h"

#include "fboss/agent/Platform.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/LacpController.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LacpTypes-defs.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>

#include <algorithm>
#include <iterator>
#include <tuple>
#include <utility>

namespace facebook {
namespace fboss {

namespace {
class ProgramForwardingState {
 public:
  ProgramForwardingState(
      PortID portID,
      AggregatePortID aggPortID,
      AggregatePort::Forwarding fwdState);
  std::shared_ptr<SwitchState> operator()(
      const std::shared_ptr<SwitchState>& state);

 private:
  PortID portID_;
  AggregatePortID aggegatePortID_;
  AggregatePort::Forwarding forwardingState_;
};

ProgramForwardingState::ProgramForwardingState(
    PortID portID,
    AggregatePortID aggPortID,
    AggregatePort::Forwarding fwdState)
    : portID_(portID), aggegatePortID_(aggPortID), forwardingState_(fwdState) {}

std::shared_ptr<SwitchState> ProgramForwardingState::operator()(
    const std::shared_ptr<SwitchState>& state) {
  std::shared_ptr<SwitchState> nextState(state);
  auto* aggPort =
      nextState->getAggregatePorts()->getAggregatePortIf(aggegatePortID_).get();
  if (!aggPort) {
    return nullptr;
  }

  XLOG(DBG4) << "Updating AggregatePort " << aggPort->getID()
             << ": ForwardingState[" << portID_ << "] --> "
             << (forwardingState_ == AggregatePort::Forwarding::ENABLED
                     ? "ENABLED"
                     : "DISABLED");

  aggPort = aggPort->modify(&nextState);
  aggPort->setForwardingState(portID_, forwardingState_);

  return nextState;
}
} // namespace

// Needed for CHECK_* macros to work with PortIDToController::iterator
std::ostream& operator<<(
    std::ostream& out,
    const LinkAggregationManager::PortIDToController::iterator& it) {
  return (out << it.get_ptr());
}

LinkAggregationManager::LinkAggregationManager(SwSwitch* sw)
    : AutoRegisterStateObserver(sw, "LinkAggregationManager"),
      portToController_(),
      sw_(sw) {}

void LinkAggregationManager::handlePacket(
    std::unique_ptr<RxPacket> pkt,
    folly::io::Cursor c) {
  // TODO(samank): check this is running in RX thread?
  folly::SharedMutexWritePriority::ReadHolder g(&controllersLock_);
  auto ingressPort = pkt->getSrcPort();

  auto it = portToController_.find(ingressPort);
  if (it == portToController_.end()) {
    XLOG(ERR) << "No LACP controller found for port " << ingressPort;
    return;
  }

  auto lacpdu = LACPDU::from(&c);
  if (!lacpdu.isValid()) {
    XLOG(ERR) << "Invalid LACP data unit";
    return;
  }

  it->second->received(lacpdu);
}

void LinkAggregationManager::stateUpdated(const StateDelta& delta) {
  CHECK(sw_->getUpdateEvb()->inRunningEventBaseThread());

  folly::SharedMutexWritePriority::WriteHolder writeGuard(&controllersLock_);

  DeltaFunctions::forEachChanged(
      delta.getAggregatePortsDelta(),
      &LinkAggregationManager::aggregatePortChanged,
      &LinkAggregationManager::aggregatePortAdded,
      &LinkAggregationManager::aggregatePortRemoved,
      this);

  // Downgrade to a reader lock
  folly::SharedMutexWritePriority::ReadHolder readGuard(std::move(writeGuard));

  DeltaFunctions::forEachChanged(
      delta.getPortsDelta(), &LinkAggregationManager::portChanged, this);
}

void LinkAggregationManager::aggregatePortAdded(
    const std::shared_ptr<AggregatePort>& aggPort) {
  PortIDToController::iterator it;
  bool inserted;

  for (const auto& subport : aggPort->sortedSubports()) {
    std::tie(it, inserted) = portToController_.emplace(
        subport.portID,
        std::make_shared<LacpController>(
            subport.portID,
            sw_->getLacpEvb(),
            subport.priority,
            subport.rate,
            subport.activity,
            aggPort->getID(),
            aggPort->getSystemPriority(),
            aggPort->getSystemID(),
            aggPort->getMinimumLinkCount(),
            this));
    CHECK(inserted);
    it->second->startMachines();
  }
}

void LinkAggregationManager::aggregatePortRemoved(
    const std::shared_ptr<AggregatePort>& aggPort) {
  PortIDToController::iterator it;
  for (const auto& subport : aggPort->sortedSubports()) {
    it = portToController_.find(subport.portID);
    CHECK_NE(it, portToController_.end());
    it->second->stopMachines();
    portToController_.erase(it);
  }
}

void LinkAggregationManager::aggregatePortChanged(
    const std::shared_ptr<AggregatePort>& oldAggPort,
    const std::shared_ptr<AggregatePort>& newAggPort) {
  CHECK_EQ(oldAggPort->getID(), newAggPort->getID());

  auto oldSubportRange = oldAggPort->sortedSubports();
  if (oldAggPort->getName() == newAggPort->getName() &&
      oldAggPort->getDescription() == newAggPort->getDescription() &&
      oldAggPort->getSystemPriority() == newAggPort->getSystemPriority() &&
      oldAggPort->getSystemID() == newAggPort->getSystemID() &&
      std::equal(
          oldSubportRange.begin(),
          oldSubportRange.end(),
          newAggPort->sortedSubports().begin())) {
    return;
  }

  aggregatePortRemoved(oldAggPort);
  aggregatePortAdded(newAggPort);
}

void LinkAggregationManager::portChanged(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  auto portId = newPort->getID();

  if (oldPort->getOperState() == Port::OperState::DOWN &&
      newPort->getOperState() == Port::OperState::UP) {
    auto it = portToController_.find(portId);
    if (it == portToController_.end()) {
      XLOG(ERR) << "Port " << portId << " not found";
      return;
    }
    it->second->portUp();
  } else if (
      oldPort->getOperState() == Port::OperState::UP &&
      newPort->getOperState() == Port::OperState::DOWN) {
    auto it = portToController_.find(portId);
    if (it == portToController_.end()) {
      XLOG(ERR) << "Port " << portId << " not found";
      return;
    }
    it->second->portDown();
  }
}

void LinkAggregationManager::populatePartnerPair(
    PortID portID,
    LacpPartnerPair& partnerPair) {
  // TODO(samank): CHECK in Thrift worker thread

  std::shared_ptr<LacpController> controller;
  {
    folly::SharedMutexWritePriority::ReadHolder g(&controllersLock_);

    auto it = portToController_.find(portID);
    if (it == portToController_.end()) {
      throw LACPError("No LACP controller found for port ", portID);
    }

    controller = it->second;
  }

  controller->actorInfo().populate(partnerPair.localEndpoint);
  controller->partnerInfo().populate(partnerPair.remoteEndpoint);
}

void LinkAggregationManager::populatePartnerPairs(
    std::vector<LacpPartnerPair>& partnerPairs) {
  // TODO(samank): CHECK in Thrift worker thread

  partnerPairs.clear();

  folly::SharedMutexWritePriority::ReadHolder g(&controllersLock_);

  partnerPairs.reserve(
      std::distance(portToController_.begin(), portToController_.end()));

  for (const auto& portAndController : portToController_) {
    auto controller = portAndController.second;

    partnerPairs.emplace_back();

    controller->actorInfo().populate(partnerPairs.back().localEndpoint);
    controller->partnerInfo().populate(partnerPairs.back().remoteEndpoint);
  }
}

bool LinkAggregationManager::transmit(LACPDU lacpdu, PortID portID) {
  CHECK(sw_->getLacpEvb()->inRunningEventBaseThread());

  auto pkt = sw_->allocatePacket(LACPDU::LENGTH);
  if (!pkt) {
    XLOG(DBG4) << "Failed to allocate tx packet for LACPDU transmission";
    return false;
  }

  folly::io::RWPrivateCursor writer(pkt->buf());

  folly::MacAddress cpuMac = sw_->getPlatform()->getLocalMac();

  auto port = sw_->getState()->getPorts()->getPortIf(portID);
  CHECK(port);

  TxPacket::writeEthHeader(
      &writer,
      LACPDU::kSlowProtocolsDstMac(),
      cpuMac,
      port->getIngressVlan(),
      LACPDU::EtherType::SLOW_PROTOCOLS);

  writer.writeBE<uint8_t>(LACPDU::EtherSubtype::LACP);

  lacpdu.to(&writer);

  sw_->sendPacketOutOfPort(std::move(pkt), portID);

  return true;
}

void LinkAggregationManager::enableForwarding(
    PortID portID,
    AggregatePortID aggPortID) {
  CHECK(sw_->getLacpEvb()->inRunningEventBaseThread());

  auto enableFwdStateFn = ProgramForwardingState(
      portID, aggPortID, AggregatePort::Forwarding::ENABLED);

  sw_->updateStateNoCoalescing(
      "AggregatePort ForwardingState", std::move(enableFwdStateFn));
}

void LinkAggregationManager::disableForwarding(
    PortID portID,
    AggregatePortID aggPortID) {
  CHECK(sw_->getLacpEvb()->inRunningEventBaseThread());

  auto disableFwdStateFn = ProgramForwardingState(
      portID, aggPortID, AggregatePort::Forwarding::DISABLED);

  sw_->updateStateNoCoalescing(
      "AggregatePort ForwardingState", std::move(disableFwdStateFn));
}

std::vector<std::shared_ptr<LacpController>>
LinkAggregationManager::getControllersFor(
    folly::Range<std::vector<PortID>::const_iterator> ports) {
  // Although this method is thread-safe, it is only invoked from a Selector
  // object, which should always be executing over the LACP EVB
  CHECK(sw_->getLacpEvb()->inRunningEventBaseThread());

  std::vector<std::shared_ptr<LacpController>> controllers(
      std::distance(ports.begin(), ports.end()));

  folly::SharedMutexWritePriority::ReadHolder g(&controllersLock_);

  // TODO(samank): Rerwite as an O(N + M) algorithm
  for (auto i = 0; i < controllers.size(); ++i) {
    auto it = portToController_.find(ports[i]);
    CHECK(it != portToController_.end());

    controllers[i] = it->second;
  }

  // TODO(samank): does this move?
  return controllers;
}

LinkAggregationManager::~LinkAggregationManager() {}
} // namespace fboss
} // namespace facebook
