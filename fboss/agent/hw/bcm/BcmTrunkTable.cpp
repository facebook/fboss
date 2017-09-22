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

#include "BcmSwitch.h"
#include "BcmTrunk.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/state/AggregatePort.h"

namespace facebook {
namespace fboss {

BcmTrunkTable::BcmTrunkTable(const BcmSwitch* hw) : trunks_(), hw_(hw) {}

BcmTrunkTable::~BcmTrunkTable() {}

void BcmTrunkTable::addTrunk(const std::shared_ptr<AggregatePort>& aggPort) {
  setupTrunking();
  auto trunk = std::make_unique<BcmTrunk>(hw_);
  trunk->init(aggPort);

  bool inserted;
  std::tie(std::ignore, inserted) =
      trunks_.insert(std::make_pair(aggPort->getID(), std::move(trunk)));

  if (!inserted) {
    throw FbossError(
        "failed to add aggregate port ",
        aggPort->getID(),
        ": corresponding trunk already exists");
  }
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
}

void BcmTrunkTable::deleteTrunk(const std::shared_ptr<AggregatePort>& aggPort) {
  auto it = trunks_.find(aggPort->getID());
  if (it == trunks_.end()) {
    throw FbossError(
        "failed to delete aggregate port ",
        aggPort->getID(),
        ": no corresponding trunk");
  }

  trunks_.erase(it);
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
    LOG(INFO) << "Did not find trunk corresponding to port " << port;
    return facebook::fboss::BcmTrunk::INVALID;
  }
  auto trunk = *maybeTrunk;

  // Note that getEnabledMemberPortsCountHwNotLocked() must be invoked before
  // shrinkTrunkGroupHwNotLocked()
  auto count = BcmTrunk::getEnabledMemberPortsCountHwNotLocked(
      hw_->getUnit(), trunk, port);
  LOG(INFO) << count << " member ports enabled in trunk " << trunk;
  BcmTrunk::shrinkTrunkGroupHwNotLocked(hw_->getUnit(), trunk, port);

  if (count > 1) { // (1.b)
    return facebook::fboss::BcmTrunk::INVALID;
  }

  return trunk; // (2)
}
}
} // namespace facebook::fboss
