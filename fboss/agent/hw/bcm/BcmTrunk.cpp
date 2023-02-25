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
#include <bcm/error.h>
#include <bcm/port.h>
#include <bcm/trunk.h>
}

#include <folly/container/Enumerate.h>
#include <folly/logging/xlog.h>
#include <vector>

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/state/AggregatePort.h"

using std::vector;

namespace facebook::fboss {

BcmTrunk::BcmTrunk(const BcmSwitchIf* hw) : hw_(hw), trunkStats_(hw) {}

BcmTrunk::~BcmTrunk() {
  if (bcmTrunkID_ == INVALID) {
    return;
  }

  // At least according to Broadcom's own examples, it is not necessary to
  // remove the member ports of a trunk before destroying the trunk itself
  auto rv = bcm_trunk_destroy(hw_->getUnit(), bcmTrunkID_);
  bcmLogFatal(rv, hw_, "failed to destroy trunk ", bcmTrunkID_);
  XLOG(DBG2) << "deleted trunk " << bcmTrunkID_;
}

void BcmTrunk::init(const std::shared_ptr<AggregatePort>& aggPort) {
  auto warmBootCache = hw_->getWarmBootCache();

  auto foundTrunk = std::find_if(
      warmBootCache->trunks_begin(),
      warmBootCache->trunks_end(),
      [&aggPort](auto& trunk) { return aggPort->getID() == trunk.first; });

  if (foundTrunk != warmBootCache->trunks_end()) {
    bcmTrunkID_ = foundTrunk->second;

    // Get trunk member information from SDK
    bcm_trunk_info_t info;
    bcm_trunk_info_t_init(&info);
    int member_count;
    vector<bcm_trunk_member_t> members(aggPort->forwardingSubportCount());

    auto rv = bcm_trunk_get(
        hw_->getUnit(),
        bcmTrunkID_,
        &info,
        members.size(),
        members.data(),
        &member_count);
    bcmCheckError(rv, "failed to get subports for trunk ", bcmTrunkID_);
    XLOG(DBG2) << "Found " << member_count
               << " members in HW for AggregatePort " << aggPort->getID();

    for (auto [subPort, fwdState] : aggPort->subportAndFwdState()) {
      // ignore members in sw disabled state
      if (fwdState == AggregatePort::Forwarding::DISABLED) {
        continue;
      }
      auto memberGport =
          hw_->getPortTable()->getBcmPort(subPort)->getBcmGport();
      if (none_of(members.begin(), members.end(), [&](auto& mem) {
            return mem.gport == memberGport;
          })) {
        // if a member port transitioned down before warm boot, link scan
        // will remove member from trunk and update port state. LACP
        // watches port state and removes member from Aggport. In a rare
        // scenario where warm boot shut down happened after port was
        // removed from trunk but before LACP reacts to port down, we add
        // it back here. However this will be short lived since
        // linkUp/DownHwLocked will be involked at end of warmboot init.
        XLOG(DBG2) << "Found disabled member port " << subPort;
        modifyMemberPort(true, subPort);
      } else {
        trunkStats_.grantMembership(subPort);
      }
    }
    trunkStats_.initialize(aggPort->getID(), aggPort->getName());
    warmBootCache->programmed(foundTrunk);
    return;
  }

  auto rv = bcm_trunk_create(hw_->getUnit(), 0, &bcmTrunkID_);
  bcmCheckError(
      rv, "failed to create trunk for aggregate port ", aggPort->getID());
  XLOG(DBG2) << "created trunk " << bcmTrunkID_ << " for AggregatePort "
             << aggPort->getID();

  bcm_trunk_info_t info;
  bcm_trunk_info_t_init(&info);
  info.dlf_index = BCM_TRUNK_UNSPEC_INDEX;
  info.mc_index = BCM_TRUNK_UNSPEC_INDEX;
  info.ipmc_index = BCM_TRUNK_UNSPEC_INDEX;
  info.psc = BcmTrunk::rtag7();

  vector<bcm_trunk_member_t> members(aggPort->forwardingSubportCount());

  PortID subport;
  AggregatePort::Forwarding fwdState;

  for (auto it : folly::enumerate(aggPort->subportAndFwdState())) {
    std::tie(subport, fwdState) = *it;
    if (fwdState == AggregatePort::Forwarding::DISABLED) {
      continue;
    }
    bcm_trunk_member_t_init(&members[it.index]);
    members[it.index].gport =
        hw_->getPortTable()->getBcmPort(subport)->getBcmGport();
    trunkStats_.grantMembership(subport);
  }

  rv = bcm_trunk_set(
      hw_->getUnit(), bcmTrunkID_, &info, members.size(), members.data());
  bcmCheckError(rv, "failed to set subports for trunk ", bcmTrunkID_);

  BcmTrunk::suppressTrunkInternalFlood(aggPort);

  trunkStats_.initialize(aggPort->getID(), aggPort->getName());
}

void BcmTrunk::program(
    const std::shared_ptr<AggregatePort>& oldAggPort,
    const std::shared_ptr<AggregatePort>& newAggPort,
    std::vector<PortID>& addedPorts,
    std::vector<PortID>& removedPorts) {
  programForwardingState(
      oldAggPort->subportAndFwdState(),
      newAggPort->subportAndFwdState(),
      addedPorts,
      removedPorts);

  if (oldAggPort->getName() != newAggPort->getName()) {
    trunkStats_.initialize(newAggPort->getID(), newAggPort->getName());
  }
}

void BcmTrunk::programForwardingState(
    LegacyAggregatePortFields::SubportToForwardingState oldState,
    LegacyAggregatePortFields::SubportToForwardingState newState,
    std::vector<PortID>& addedPorts,
    std::vector<PortID>& removedPorts) {
  PortID oldSubport, newSubport;
  AggregatePort::Forwarding oldFwdState, newFwdState;

  for (auto newSubportAndFwdState : newState) {
    std::tie(newSubport, newFwdState) = newSubportAndFwdState;
    auto oldSubportAndFwdStateIter = std::find_if(
        oldState.begin(),
        oldState.end(),
        [newSubport](AggregatePort::SubportAndForwardingStateValueType val) {
          return val.first == newSubport;
        });
    if (oldSubportAndFwdStateIter == oldState.end()) {
      continue;
    }
    std::tie(oldSubport, oldFwdState) = *oldSubportAndFwdStateIter;
    CHECK_EQ(oldSubport, newSubport);
    if (oldFwdState != newFwdState) {
      bool addRemove = (newFwdState == AggregatePort::Forwarding::ENABLED);
      modifyMemberPort(addRemove, newSubport);
      if (addRemove) {
        addedPorts.push_back(newSubport);
      } else {
        removedPorts.push_back(newSubport);
      }
    }
  }
}

void BcmTrunk::modifyMemberPort(bool added, PortID memberPort) {
  XLOG(DBG2) << "modifyMemberPort: bcmTrunkID_ = " << bcmTrunkID_
             << ", added = " << added << ", port = " << memberPort;
  bcm_trunk_member_t member;
  bcm_trunk_member_t_init(&member);
  member.gport = hw_->getPortTable()->getBcmPort(memberPort)->getBcmGport();
  if (added) {
    auto rv = bcm_trunk_member_add(hw_->getUnit(), bcmTrunkID_, &member);
    bcmCheckError(
        rv, "failed to add port ", memberPort, " to trunk ", bcmTrunkID_);
    trunkStats_.grantMembership(memberPort);
  } else { // deleted
    auto rv = bcm_trunk_member_delete(hw_->getUnit(), bcmTrunkID_, &member);
    if (rv == BCM_E_NOT_FOUND) {
      // The port may have already been deleted in the link-scan thread
      // and the StateDelta at hand may be the result of the SwitchState
      // catching up to the hardware state
      XLOG(DBG2) << "already deleted port " << memberPort << " from trunk "
                 << bcmTrunkID_;
      return;
    }
    bcmCheckError(
        rv, "failed to delete port ", memberPort, " from trunk ", bcmTrunkID_);
    XLOG(DBG2) << "deleted port " << memberPort << " from trunk "
               << bcmTrunkID_;
    trunkStats_.revokeMembership(memberPort);
  }
}

void BcmTrunk::shrinkTrunkGroupHwNotLocked(
    int unit,
    bcm_trunk_t trunk,
    bcm_port_t toDisable) {
  bcm_gport_t toDisableAsGPort;
  bcm_trunk_member_t member;

  auto rv = bcm_port_gport_get(unit, toDisable, &toDisableAsGPort);
  bcmCheckError(
      rv,
      "failed to get gport for bcm port ",
      toDisable,
      " in link-down context");

  bcm_trunk_member_t_init(&member);
  member.gport = static_cast<bcm_gport_t>(toDisableAsGPort);
  rv = bcm_trunk_member_delete(unit, trunk, &member);

  // Though unlikely, it is possible for the update thread to have already
  // deleted this member port from the trunk, which would cause
  // bcm_trunk_member_delete to return BCM_E_NOT_FOUND. With this in
  // mind, we ignore the BCM_E_NOT_FOUND error code here and fail hard on
  // other error codes.
  if (rv == BCM_E_NOT_FOUND) {
    return;
  }
  bcmCheckError(
      rv,
      "failed to remove port ",
      toDisable,
      " from trunk ",
      trunk,
      " in interrupt context");

  XLOG(DBG2) << "removed port " << toDisable << " from trunk " << trunk
             << " in interrupt context";
}

std::optional<bcm_trunk_t>
BcmTrunk::findTrunk(int unit, bcm_module_t modid, bcm_port_t port) {
  bcm_trunk_t trunkOut;
  auto rv = bcm_trunk_find(unit, modid, port, &trunkOut);

  if (rv == BCM_E_NOT_FOUND) {
    return std::nullopt;
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

int BcmTrunk::rtag7() {
  return BCM_TRUNK_PSC_PORTFLOW; // use RTAG7 hashing
}

void BcmTrunk::suppressTrunkInternalFlood(
    const std::shared_ptr<AggregatePort>& aggPort) {
  auto trafficToBlock = BCM_PORT_FLOOD_BLOCK_BCAST |
      BCM_PORT_FLOOD_BLOCK_UNKNOWN_UCAST | BCM_PORT_FLOOD_BLOCK_UNKNOWN_MCAST;

  for (auto ingressSubport : aggPort->sortedSubports()) {
    auto ingressPortID =
        hw_->getPortTable()->getBcmPortId(ingressSubport.portID);
    for (auto egressSubport : aggPort->sortedSubports()) {
      if (ingressSubport == egressSubport) {
        continue;
      }
      auto egressPortID =
          hw_->getPortTable()->getBcmPortId(egressSubport.portID);
      bcm_port_flood_block_set(
          hw_->getUnit(), ingressPortID, egressPortID, trafficToBlock);
    }
  }
}

bcm_gport_t BcmTrunk::asGPort(bcm_trunk_t trunk) {
  bcm_gport_t rtn;
  BCM_GPORT_TRUNK_SET(rtn, trunk);
  return rtn;
}

bool BcmTrunk::isValidTrunkPort(bcm_gport_t gport) {
  return BCM_GPORT_IS_TRUNK(gport) &&
      BCM_GPORT_TRUNK_GET(gport) != static_cast<bcm_trunk_t>(BcmTrunk::INVALID);
}

/* The Broadcom SDK uses a type of int to hold the "maximum number of
 * ports per (front panel) trunk group". Since the return value of this method
 * is a count of the _enabled_ ports in a trunk group, int is of sufficient
 * width.
 */
int BcmTrunk::getEnabledMemberPortsCountHwNotLocked(
    int unit,
    bcm_trunk_t trunk,
    bcm_port_t port) {
  bcm_pbmp_t enabledMemberPorts;
  BCM_PBMP_CLEAR(enabledMemberPorts);

  BCM_PBMP_PORT_ADD(enabledMemberPorts, port);

  auto rv = bcm_trunk_bitmap_expand(unit, &enabledMemberPorts);
  bcmCheckError(
      rv,
      "failed to retrieve enabled member ports for trunk ",
      trunk,
      " with port ",
      port);

  int enabledMemberPortsCount;
  BCM_PBMP_COUNT(enabledMemberPorts, enabledMemberPortsCount);
  return enabledMemberPortsCount;
}

} // namespace facebook::fboss
