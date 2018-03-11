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

#include "fboss/agent/LacpController.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/io/async/EventBase.h>

#include <algorithm>
#include <iterator>
#include <tuple>
#include <utility>

namespace facebook {
namespace fboss {

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
    LOG(ERROR) << "No LACP controller found for port " << ingressPort;
    return;
  }

  it->second->receivedLACPDU(c);
}

void LinkAggregationManager::stateUpdated(const StateDelta& delta) {
  CHECK(sw_->getUpdateEvb()->inRunningEventBaseThread());

  folly::SharedMutexWritePriority::WriteHolder writeGuard(&controllersLock_);

  if (!initialized_) {
    bool inserted;
    for (const auto& port : *(delta.newState()->getPorts())) {
      // TODO(samank): use try_emplace once OSS build uses boost >1.63.0
      std::tie(std::ignore, inserted) = portToController_.insert(std::make_pair(
          port->getID(),
          std::make_shared<LacpController>(
              port->getID(), sw_->getLacpEvb(), this, sw_)));
      CHECK(inserted);
    }

    for (const auto& portAndController : portToController_) {
      portAndController.second->startMachines();
    }
    initialized_ = true;
  }

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

  for (const auto& subport : aggPort->sortedSubports()) {
    it = portToController_.find(subport.portID);
    CHECK_NE(it, portToController_.end());
    it->second->stopMachines();
    it->second.reset(new LacpController(
        subport.portID,
        sw_->getLacpEvb(),
        subport.priority,
        subport.rate,
        subport.activity,
        aggPort->getID(),
        aggPort->getSystemPriority(),
        aggPort->getSystemID(),
        aggPort->getMinimumLinkCount(),
        this,
        sw_));
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
    it->second.reset(
        new LacpController(subport.portID, sw_->getLacpEvb(), this, sw_));
    it->second->startMachines();
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
      LOG(ERROR) << "Port " << portId << " not found";
      return;
    }
    it->second->portUp();
  } else if (
      oldPort->getOperState() == Port::OperState::UP &&
      newPort->getOperState() == Port::OperState::DOWN) {
    auto it = portToController_.find(portId);
    if (it == portToController_.end()) {
      LOG(ERROR) << "Port " << portId << " not found";
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

LinkAggregationManager::~LinkAggregationManager() {}
} // namespace fboss
} // namespace facebook
