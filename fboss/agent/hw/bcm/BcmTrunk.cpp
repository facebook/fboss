/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTrunk.h"

extern "C" {
#include <opennsl/error.h>
#include <opennsl/port.h>
#include <opennsl/trunk.h>
}

#include <folly/container/Enumerate.h>
#include <folly/logging/xlog.h>
#include <vector>

#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/state/AggregatePort.h"

using std::vector;

namespace facebook {
namespace fboss {

BcmTrunk::BcmTrunk(const BcmSwitch* hw) : hw_(hw), trunkStats_(hw) {}

BcmTrunk::~BcmTrunk() {
  if (bcmTrunkID_ == INVALID) {
    return;
  }

  // At least according to Broadcom's own examples, it is not necessary to
  // remove the member ports of a trunk before destroying the trunk itself
  auto rv = opennsl_trunk_destroy(hw_->getUnit(), bcmTrunkID_);
  bcmLogFatal(rv, hw_, "failed to destroy trunk ", bcmTrunkID_);
  XLOG(INFO) << "deleted trunk " << bcmTrunkID_;
}

void BcmTrunk::init(const std::shared_ptr<AggregatePort>& aggPort) {
  static int64_t nextAvailableTrunkID = 0;
  bcmTrunkID_ = nextAvailableTrunkID++;
  auto rv = opennsl_trunk_create(
    hw_->getUnit(), OPENNSL_TRUNK_FLAG_WITH_ID, &bcmTrunkID_);
  bcmCheckError(
      rv, "failed to create trunk for aggregate port ", aggPort->getID());
  XLOG(INFO) << "created trunk " << bcmTrunkID_
             << " for AggregatePort " << aggPort->getID();

  opennsl_trunk_info_t info;
  opennsl_trunk_info_t_init(&info);
  info.dlf_index = OPENNSL_TRUNK_UNSPEC_INDEX;
  info.mc_index = OPENNSL_TRUNK_UNSPEC_INDEX;
  info.ipmc_index = OPENNSL_TRUNK_UNSPEC_INDEX;
  info.psc = BcmTrunk::rtag7();

  vector<opennsl_trunk_member_t> members(aggPort->forwardingSubportCount());

  PortID subport;
  AggregatePort::Forwarding fwdState;

  for (auto it : folly::enumerate(aggPort->subportAndFwdState())) {
    std::tie(subport, fwdState) = *it;
    if (fwdState == AggregatePort::Forwarding::DISABLED) {
      continue;
    }
    opennsl_trunk_member_t_init(&members[it.index]);
    members[it.index].gport =
        hw_->getPortTable()->getBcmPort(subport)->getBcmGport();
    trunkStats_.grantMembership(subport);
  }

  rv = opennsl_trunk_set(
      hw_->getUnit(), bcmTrunkID_, &info, members.size(), members.data());
  bcmCheckError(rv, "failed to set subports for trunk ", bcmTrunkID_);

  BcmTrunk::suppressTrunkInternalFlood(aggPort);

  trunkStats_.initialize(aggPort->getID(), aggPort->getName());
}

void BcmTrunk::program(
    const std::shared_ptr<AggregatePort>& oldAggPort,
    const std::shared_ptr<AggregatePort>& newAggPort) {
  programForwardingState(oldAggPort->subportAndFwdState(),
                         newAggPort->subportAndFwdState());

  if (oldAggPort->getName() != newAggPort->getName()) {
    trunkStats_.initialize(newAggPort->getID(), newAggPort->getName());
  }
}

void BcmTrunk::programForwardingState(
     AggregatePort::SubportAndForwardingStateConstRange oldRange,
     AggregatePort::SubportAndForwardingStateConstRange newRange) {
  PortID oldSubport, newSubport;
  AggregatePort::Forwarding oldFwdState, newFwdState;

  for (auto newSubportAndFwdState : newRange) {
    std::tie(newSubport, newFwdState) = newSubportAndFwdState;
    auto oldSubportAndFwdStateIter = std::find_if(
        oldRange.begin(),
        oldRange.end(),
        [newSubport](AggregatePort::SubportAndForwardingStateValueType val) {
          return val.first == newSubport;
        });
    if (oldSubportAndFwdStateIter == oldRange.end()) {
      continue;
    }
    std::tie(oldSubport, oldFwdState) = *oldSubportAndFwdStateIter;
    CHECK_EQ(oldSubport, newSubport);
    if (oldFwdState != newFwdState) {
      modifyMemberPort(
          newFwdState == AggregatePort::Forwarding::ENABLED, newSubport);
    }
  }
}

void BcmTrunk::modifyMemberPort(bool added, PortID memberPort) {
  opennsl_trunk_member_t member;
  opennsl_trunk_member_t_init(&member);
  member.gport = hw_->getPortTable()->getBcmPort(memberPort)->getBcmGport();
  if (added) {
    auto rv = opennsl_trunk_member_add(hw_->getUnit(), bcmTrunkID_, &member);
    bcmCheckError(
        rv, "failed to add port ", memberPort, " to trunk ", bcmTrunkID_);
    XLOG(INFO) << "added port " << memberPort << " to trunk " << bcmTrunkID_;
    trunkStats_.grantMembership(memberPort);
  } else { // deleted
    auto rv = opennsl_trunk_member_delete(hw_->getUnit(), bcmTrunkID_, &member);
    if (rv == OPENNSL_E_NOT_FOUND) {
      // The port may have already been deleted in the link-scan thread
      // and the StateDelta at hand may be the result of the SwitchState
      // catching up to the hardware state
      XLOG(INFO) << "already deleted port " << memberPort << " from trunk "
                 << bcmTrunkID_;
      return;
    }
    bcmCheckError(
        rv, "failed to delete port ", memberPort, " from trunk ", bcmTrunkID_);
    XLOG(INFO) << "deleted port " << memberPort << " from trunk "
               << bcmTrunkID_;
    trunkStats_.revokeMembership(memberPort);
  }
}

void BcmTrunk::shrinkTrunkGroupHwNotLocked(
    int unit,
    opennsl_trunk_t trunk,
    opennsl_port_t toDisable) {
  opennsl_gport_t toDisableAsGPort;
  opennsl_trunk_member_t member;

  auto rv = opennsl_port_gport_get(unit, toDisable, &toDisableAsGPort);
  bcmCheckError(
      rv,
      "failed to get gport for bcm port ",
      toDisable,
      " in link-down context");

  opennsl_trunk_member_t_init(&member);
  member.gport = static_cast<opennsl_gport_t>(toDisableAsGPort);
  rv = opennsl_trunk_member_delete(unit, trunk, &member);

  // Though unlikely, it is possible for the update thread to have already
  // deleted this member port from the trunk, which would cause
  // opennsl_trunk_member_delete to return OPENNSL_E_NOT_FOUND. With this in
  // mind, we ignore the OPENNSL_E_NOT_FOUND error code here and fail hard on
  // other error codes.
  if (rv == OPENNSL_E_NOT_FOUND) {
    return;
  }
  bcmCheckError(
      rv,
      "failed to remove port ",
      toDisable,
      " from trunk ",
      trunk,
      " in interrupt context");

  XLOG(INFO) << "removed port " << toDisable << " from trunk " << trunk
             << " in interrupt context";
}

folly::Optional<opennsl_trunk_t>
BcmTrunk::findTrunk(int unit, opennsl_module_t modid, opennsl_port_t port) {
  opennsl_trunk_t trunkOut;
  auto rv = opennsl_trunk_find(unit, modid, port, &trunkOut);

  if (rv == OPENNSL_E_NOT_FOUND) {
    return folly::none;
  }
  bcmCheckError(
      rv,
      "failed to find trunk corresponding to port ",
      port,
      " in link-down context");

  return trunkOut;
}

BcmTrunkStats& BcmTrunk::stats() {
  return trunkStats_;
}

} // namespace fboss
} // namespace facebook
