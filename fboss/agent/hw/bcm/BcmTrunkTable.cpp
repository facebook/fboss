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
#include <bcm/trunk.h>
}

#include "BcmSwitch.h"
#include "BcmTrunk.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/state/AggregatePort.h"

namespace facebook::fboss {

BcmTrunkTable::BcmTrunkTable(const BcmSwitch* hw)
    : trunks_(), hw_(hw), trunkToMinLinkCount_(), tgidLookup_() {}

BcmTrunkTable::~BcmTrunkTable() {}

bcm_trunk_t BcmTrunkTable::getBcmTrunkId(AggregatePortID id) const {
  auto iter = tgidLookup_.find(id);
  if (iter == tgidLookup_.end()) {
    throw FbossError("Cannot find the BCM trunk id for aggregatePort ", id);
  }
  return iter->second;
}

AggregatePortID BcmTrunkTable::getAggregatePortId(bcm_trunk_t trunk) const {
  for (const auto [aggregateID, trunkID] : tgidLookup_) {
    if (trunkID == trunk) {
      return aggregateID;
    }
  }
  throw FbossError("Cannot find the aggregatePort id for trunk ", trunk);
}

void BcmTrunkTable::addTrunk(const std::shared_ptr<AggregatePort>& aggPort) {
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
  tgidLookup_.insert(aggPort->getID(), trunkID);
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
  tgidLookup_.erase(aggPort->getID());
}

/* 1. If bcm_trunk_t == INVALID, then
 *    a. port does _not_ belong to any trunk port, OR
 *    b. port _does_ belong to a trunk port, but the loss of the port does not
 *       affect layer three reachability (ie. there is at least one physical
 *       member port up)
 * 2. If bcm_trunk_t != INVALID, then all ECMP egress entries which egress
 *    over this physical port must be shrunk to exclude it.
 */
bcm_trunk_t BcmTrunkTable::linkDownHwNotLocked(bcm_port_t port) const {
  auto maybeTrunk =
      BcmTrunk::findTrunk(hw_->getUnit(), static_cast<bcm_module_t>(0), port);

  if (!maybeTrunk) { // (1.a)
    return facebook::fboss::BcmTrunk::INVALID;
  }
  auto trunk = *maybeTrunk;
  XLOG(INFO) << "Found trunk " << trunk << " for port " << port;

  // Note that getEnabledMemberPortsCountHwNotLocked() must be invoked before
  // shrinkTrunkGroupHwNotLocked()
  auto count = BcmTrunk::getEnabledMemberPortsCountHwNotLocked(
      hw_->getUnit(), trunk, port);
  XLOG(INFO) << count << " member ports enabled in trunk " << trunk;
  BcmTrunk::shrinkTrunkGroupHwNotLocked(hw_->getUnit(), trunk, port);

  auto maybeMinLinkCount = trunkToMinLinkCount_.get(trunk);
  if (!maybeMinLinkCount) {
    XLOG(WARNING) << "Trunk " << trunk
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

bool BcmTrunkTable::isMinLinkMet(bcm_trunk_t trunk) const {
  auto maybeMinLinkCount = trunkToMinLinkCount_.get(trunk);
  if (!maybeMinLinkCount) {
    return false;
  }
  auto minLinkCount = *maybeMinLinkCount;

  bcm_trunk_info_t info;
  bcm_trunk_info_t_init(&info);
  int memberCount;
  std::vector<bcm_trunk_member_t> members(BCM_TRUNK_MAX_PORTCNT);

  auto rv = bcm_trunk_get(
      hw_->getUnit(),
      trunk,
      &info,
      members.size(),
      members.data(),
      &memberCount);
  bcmCheckError(rv, "failed to get subports for trunk ", trunk);
  if (!memberCount) {
    return false;
  }

  auto memGport = members[0].gport & 0x3FFFFFF;
  auto count = BcmTrunk::getEnabledMemberPortsCountHwNotLocked(
      hw_->getUnit(), trunk, memGport);
  XLOG(INFO) << count << " member ports enabled in trunk " << trunk;

  return count < minLinkCount ? false : true;
}

} // namespace facebook::fboss
