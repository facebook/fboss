/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmTeFlowEntry.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTeFlowTable.h"

#include <boost/container/flat_map.hpp>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/state/TeFlowEntry.h"

extern "C" {
#include <bcm/field.h>
}

namespace facebook::fboss {

using facebook::network::toIPAddress;

void BcmTeFlowEntry::createTeFlowEntry() {
  bcm_if_t egressId;
  /* EM Entry Configuration and Creation */
  auto rv = bcm_field_entry_create(hw_->getUnit(), gid_, &handle_);
  bcmCheckError(rv, "failed to create field entry");

  if (teFlow_->getFlow().srcPort().has_value()) {
    auto srcPort = teFlow_->getFlow().srcPort().value();
    rv = bcm_field_qualify_SrcPort(
        hw_->getUnit(), handle_, 0, 0xff, srcPort, 0xff);
    bcmCheckError(rv, "failed to qualify src port");
  }

  if (teFlow_->getFlow().dstPrefix().has_value()) {
    bcm_ip6_t addr;
    bcm_ip6_t mask{};
    auto prefix = teFlow_->getFlow().dstPrefix().value();
    folly::IPAddress ipaddr = toIPAddress(*prefix.ip());
    uint8_t pfxLen = static_cast<uint8_t>(*prefix.prefixLength());
    if (!ipaddr.isV6()) {
      throw FbossError("only ipv6 addresses are supported for teflow");
    }
    facebook::fboss::ipToBcmIp6(ipaddr, &addr);
    memcpy(&mask, folly::IPAddressV6::fetchMask(pfxLen).data(), sizeof(mask));
    rv = bcm_field_qualify_DstIp6(hw_->getUnit(), handle_, addr, mask);
    bcmCheckError(rv, "failed to qualify dst Ip6");
  }

  auto nhts = teFlow_->getResolvedNextHops();
  RouteNextHopSet nexthops = util::toRouteNextHopSet(nhts);
  if (nexthops.size() > 0) {
    redirectNexthop_ =
        hw_->writableMultiPathNextHopTable()->referenceOrEmplaceNextHop(
            BcmMultiPathNextHopKey(0, nexthops));
    egressId = redirectNexthop_->getEgressId();
  } else {
    egressId = hw_->getDropEgressId();
  }

  rv = bcm_field_action_add(
      hw_->getUnit(), handle_, bcmFieldActionL3Switch, egressId, 0);
  bcmCheckError(rv, "failed to action add");

  rv = bcm_field_entry_install(hw_->getUnit(), handle_);
  bcmCheckError(rv, "failed to install field group");

  int enabled = teFlow_->getEnabled() ? 1 : 0;
  rv = bcm_field_entry_enable_set(hw_->getUnit(), handle_, enabled);
  bcmCheckError(
      rv, "Failed to set enable/disable for teflow. enable:", enabled);
}

BcmTeFlowEntry::BcmTeFlowEntry(
    BcmSwitch* hw,
    int gid,
    const std::shared_ptr<TeFlowEntry>& teFlow)
    : hw_(hw), gid_(gid), teFlow_(teFlow) {
  createTeFlowEntry();
}

BcmTeFlowEntry::~BcmTeFlowEntry() {
  int rv;

  // Destroy the TeFlow entry
  rv = bcm_field_entry_destroy(hw_->getUnit(), handle_);
  bcmLogFatal(rv, hw_, "failed to destroy the TeFlow entry");
}

TeFlow BcmTeFlowEntry::getID() const {
  return teFlow_->getID();
}

} // namespace facebook::fboss
