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

#include "BcmTrunk.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/AggregatePort.h"

namespace facebook {
namespace fboss {

BcmTrunkTable::BcmTrunkTable(const BcmSwitch* hw) : trunks_(), hw_(hw) {}

BcmTrunkTable::~BcmTrunkTable() {}

void BcmTrunkTable::addTrunk(const std::shared_ptr<AggregatePort>& aggPort) {
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
}
} // namespace facebook::fboss
