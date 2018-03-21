/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmTrunkTable.h"

extern "C" {
#include <opennsl/trunk.h>
}

#include "BcmSwitch.h"
#include "BcmTrunk.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/state/AggregatePort.h"

namespace facebook {
namespace fboss {

BcmTrunkTable::BcmTrunkTable(const BcmSwitch* hw)
    : trunks_(), hw_(hw), trunkToMinLinkCount_() {}

BcmTrunkTable::~BcmTrunkTable() {}

void BcmTrunkTable::setupTrunking() {
  if (!isBcmHWTrunkInitialized_) {
    auto rv = opennsl_trunk_init(hw_->getUnit());
    bcmCheckError(rv, "Failed to initialize trunking machinery");
    isBcmHWTrunkInitialized_ = true;
  }
}

void BcmTrunkTable::addTrunk(const std::shared_ptr<AggregatePort>& aggPort) {
  setupTrunking();
  auto trunk = std::make_unique<BcmTrunk>(hw_);
  trunk->init(aggPort);
  auto trunkID = trunk->id();

  bool inserted;
  std::tie(std::ignore, inserted) =
      trunks_.insert(std::make_pair(aggPort->getID(), std::move(trunk)));

  if (!inserted) {
    throw FbossError(
        "failed to add aggregate port ",
        aggPort->getID(),
        ": corresponding trunk already exists");
  }

  trunkToMinLinkCount_.addOrUpdate(trunkID, aggPort->getMinimumLinkCount());
}

void BcmTrunkTable::programTrunk(
    const std::shared_ptr<AggregatePort>& oldAggPort,
    const std::shared_ptr<AggregatePort>& newAggPort) {
  CHECK_EQ(oldAggPort->getID(), newAggPort->getID());

  auto it = trunks_.find(oldAggPort->getID());
  if (it == trunks_.end()) {
    throw FbossError(
        "failed to program aggregate port ",
        oldAggPort->getID(),
        ": no corresponding trunk");
  }

  it->second->program(oldAggPort, newAggPort);
  trunkToMinLinkCount_.addOrUpdate(
      it->second->id(), newAggPort->getMinimumLinkCount());
}

void BcmTrunkTable::deleteTrunk(const std::shared_ptr<AggregatePort>& aggPort) {
  auto it = trunks_.find(aggPort->getID());
  if (it == trunks_.end()) {
    throw FbossError(
        "failed to delete aggregate port ",
        aggPort->getID(),
        ": no corresponding trunk");
  }

  auto trunkID = it->second->id();
  trunks_.erase(it);
  trunkToMinLinkCount_.del(trunkID);
}

/* 1. If opennsl_trunk_t == INVALID, then
 *    a. port does _not_ belong to any trunk port, OR
 *    b. port _does_ belong to a trunk port, but the loss of the port does not
 *       affect layer three reachability (ie. there is at least one physical
 *       member port up)
 * 2. If opennsl_trunk_t != INVALID, then all ECMP egress entries which egress
 *    over this physical port must be shrunk to exclude it.
 */
opennsl_trunk_t BcmTrunkTable::linkDownHwNotLocked(opennsl_port_t port) {
  auto maybeTrunk = BcmTrunk::findTrunk(
      hw_->getUnit(), static_cast<opennsl_module_t>(0), port);

  if (!maybeTrunk) { // (1.a)
    return facebook::fboss::BcmTrunk::INVALID;
  }
  auto trunk = *maybeTrunk;
  LOG(INFO) << "Found trunk " << trunk << " for port " << port;

  // Note that getEnabledMemberPortsCountHwNotLocked() must be invoked before
  // shrinkTrunkGroupHwNotLocked()
  auto count = BcmTrunk::getEnabledMemberPortsCountHwNotLocked(
      hw_->getUnit(), trunk, port);
  LOG(INFO) << count << " member ports enabled in trunk " << trunk;
  BcmTrunk::shrinkTrunkGroupHwNotLocked(hw_->getUnit(), trunk, port);

  auto maybeMinLinkCount = trunkToMinLinkCount_.get(trunk);
  if (!maybeMinLinkCount) {
    LOG(WARNING) << "Trunk " << trunk
                 << " removed out from underneath linkscan thread";
    return facebook::fboss::BcmTrunk::INVALID;
  }
  auto minLinkCount = *maybeMinLinkCount;

  if (count > minLinkCount) { // (1.b)
    return facebook::fboss::BcmTrunk::INVALID;
  }

  return trunk; // (2)
}

void BcmTrunkTable::updateStats() {
  for (const auto& idAndTrunk : trunks_) {
    BcmTrunk* trunk = idAndTrunk.second.get();
    trunk->stats().update();
  }
}
} // namespace fboss
} // namespace facebook
