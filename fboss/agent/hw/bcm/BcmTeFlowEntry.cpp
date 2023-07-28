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
#include <exception>
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmExactMatchUtils.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTeFlowTable.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/state/TeFlowEntry.h"

extern "C" {
#include <bcm/field.h>
}

namespace facebook::fboss {

using facebook::network::toIPAddress;
using namespace facebook::fboss::utility;

void BcmTeFlowEntry::setRedirectNexthop(
    const BcmSwitch* hw,
    std::shared_ptr<BcmMultiPathNextHop>& redirectNexthop,
    const std::shared_ptr<TeFlowEntry>& teFlow) {
  // THRIFT_COPY: investigate if this can be done withought thrift copy
  auto nhts = teFlow->getResolvedNextHops()->toThrift();
  RouteNextHopSet nexthops = util::toRouteNextHopSet(nhts, true);
  if (nexthops.size() > 0) {
    redirectNexthop =
        hw->writableMultiPathNextHopTable()->referenceOrEmplaceNextHop(
            BcmMultiPathNextHopKey(0, nexthops));
  } else {
    redirectNexthop = nullptr;
  }
}

bcm_if_t BcmTeFlowEntry::getEgressId(
    const BcmSwitch* hw,
    std::shared_ptr<BcmMultiPathNextHop>& redirectNexthop) {
  bcm_if_t egressId;
  if (redirectNexthop) {
    egressId = redirectNexthop->getEgressId();
  } else {
    egressId = hw->getDropEgressId();
  }

  return egressId;
}

// getEnabled() check is needed for warmboot
// since statEnabled field is not set in the old state
// TODO remove the getEnabled() check once migrated to new state
bool BcmTeFlowEntry::isStatEnabled(const std::shared_ptr<TeFlowEntry>& teFlow) {
  return teFlow->getStatEnabled().has_value()
      ? teFlow->getStatEnabled().value()
      : (teFlow->getEnabled() && teFlow->getCounterID().has_value());
}

void BcmTeFlowEntry::createTeFlowQualifiers() {
  int rv;
  const auto& flow = teFlow_->getFlow();
  if (const auto& srcPort = flow->cref<ctrl_if_tags::srcPort>()) {
    rv = bcm_field_qualify_SrcPort(
        hw_->getUnit(), handle_, 0, 0xff, srcPort->toThrift(), 0xff);
    bcmCheckError(rv, teFlow_->str(), "failed to qualify src port");
  }

  if (const auto& dstPrefix = flow->cref<ctrl_if_tags::dstPrefix>()) {
    bcm_ip6_t addr;
    bcm_ip6_t mask{};
    auto prefix = dstPrefix->toThrift();
    folly::IPAddress ipaddr = toIPAddress(*prefix.ip());
    // Bcm SDK API for EM takes only full mask.Hence pfxLen is 128.
    uint8_t pfxLen = 128;
    if (!ipaddr.isV6()) {
      throw FbossError("only ipv6 addresses are supported for teflow");
    }
    facebook::fboss::ipToBcmIp6(ipaddr, &addr);
    memcpy(&mask, folly::IPAddressV6::fetchMask(pfxLen).data(), sizeof(mask));
    rv = bcm_field_qualify_DstIp6(hw_->getUnit(), handle_, addr, mask);
    bcmCheckError(rv, teFlow_->str(), "failed to qualify dst Ip6");
  }
}

void BcmTeFlowEntry::createTeFlowStat(
    const std::shared_ptr<TeFlowEntry>& teFlow) {
  auto teFlowTable = hw_->writableTeFlowTable();
  auto counterName = teFlow->getCounterID()->toThrift();
  const auto warmBootCache = hw_->getWarmBootCache();
  auto warmBootItr = warmBootCache->findTeFlowStat(handle_);
  if (warmBootItr != warmBootCache->TeFlowEntry2TeFlowStat_end()) {
    // If we are re-using an existing stat, call programmed() to indicate
    // that it has been claimed and must not be detached when cleaning the
    // warmboot cache
    teFlowTable->incRefOrCreateBcmTeFlowStat(
        counterName, warmBootItr->second.stat, warmBootItr->second.actionIndex);
    warmBootCache->programmed(warmBootItr);
  } else {
    auto teFlowStat =
        teFlowTable->incRefOrCreateBcmTeFlowStat(counterName, gid_);
    teFlowStat->attach(handle_);
  }
}

void BcmTeFlowEntry::deleteTeFlowStat(
    const std::shared_ptr<TeFlowEntry>& teFlow) {
  if (isStatEnabled(teFlow)) {
    auto counterName = teFlow->getCounterID()->toThrift();
    auto teFlowTable = hw_->writableTeFlowTable();
    auto teFlowStat = teFlowTable->getTeFlowStat(counterName);
    teFlowStat->detach(handle_);
    teFlowTable->derefBcmTeFlowStat(counterName);
  }
}

void BcmTeFlowEntry::createTeFlowActions() {
  setRedirectNexthop(hw_, redirectNexthop_, teFlow_);
  auto egressId = getEgressId(hw_, redirectNexthop_);
  XLOG(DBG3) << "Setting egress Id:" << egressId << " for handle :" << handle_;
  auto rv = bcm_field_action_add(
      hw_->getUnit(), handle_, bcmFieldActionL3Switch, egressId, 0);
  bcmCheckError(rv, teFlow_->str(), "failed to action add");
}

void BcmTeFlowEntry::removeTeFlowActions() {
  XLOG(DBG3) << "Remove TeFlow action for handle :" << handle_;
  auto rv =
      bcm_field_action_remove(hw_->getUnit(), handle_, bcmFieldActionL3Switch);
  bcmCheckError(rv, teFlow_->str(), "failed to action remove ");
}

void BcmTeFlowEntry::installTeFlowEntry() {
  XLOG(DBG3) << "Installing TeFlow entry for handle :" << handle_;
  auto rv = bcm_field_entry_install(hw_->getUnit(), handle_);
  bcmCheckError(rv, teFlow_->str(), "failed to install field group");
}

void BcmTeFlowEntry::createTeFlowEntry() {
  bool statAllocated{false};
  /* EM Entry Configuration and Creation */
  auto rv =
      bcm_field_entry_create(hw_->getUnit(), getEMGroupID(gid_), &handle_);
  // FIXME - SDK give resource error instead of table full
  // Remove this logic after CS00012269235 & CS00012268802 are fixed
  if (rv == BCM_E_RESOURCE) {
    throw BcmError(
        BCM_E_FULL,
        "Failed to created field entry. Exact Match entries exhausted");
  }
  bcmCheckError(rv, teFlow_->str(), "failed to create field entry");

  createTeFlowQualifiers();
  int enabled = teFlow_->getEnabled() ? 1 : 0;
  // enable/disable per entry is not supported in EM. Hence we are relying on
  // actions.
  try {
    if (enabled) {
      createTeFlowActions();
    }
    // Allocate stat entry if stat is enabled for teflow
    if (isStatEnabled(teFlow_)) {
      createTeFlowStat(teFlow_);
      statAllocated = true;
    }
    installTeFlowEntry();
  } catch (const facebook::fboss::BcmError& error) {
    if (statAllocated) {
      deleteTeFlowStat(teFlow_);
    }
    rv = bcm_field_entry_destroy(hw_->getUnit(), handle_);
    bcmLogFatal(rv, hw_, "failed to destroy TeFlow entry");
    // FIXME - SDK give resource error instead of table full
    // Remove this logic after CS00012269235 & CS00012268802 are fixed
    if (error.getBcmError() == BCM_E_RESOURCE) {
      throw BcmError(BCM_E_FULL, "Exact Match entries exhausted");
    }
    throw;
  }
}

void BcmTeFlowEntry::updateStats(
    const std::shared_ptr<TeFlowEntry>& oldTeFlow,
    const std::shared_ptr<TeFlowEntry>& newTeFlow) {
  if (!isStatEnabled(oldTeFlow) && !isStatEnabled(newTeFlow)) {
    return;
  }
  if (!isStatEnabled(oldTeFlow) && isStatEnabled(newTeFlow)) {
    createTeFlowStat(newTeFlow);
    return;
  }
  if (isStatEnabled(oldTeFlow) && !isStatEnabled(newTeFlow)) {
    deleteTeFlowStat(oldTeFlow);
    return;
  }
  if (isStatEnabled(oldTeFlow) && isStatEnabled(newTeFlow)) {
    bool isCounterIdSame =
        (oldTeFlow->getCounterID() == newTeFlow->getCounterID());
    if (isCounterIdSame) {
      return;
    }
    deleteTeFlowStat(oldTeFlow);
    createTeFlowStat(newTeFlow);
  }
}

void BcmTeFlowEntry::updateTeFlowEntry(
    const std::shared_ptr<TeFlowEntry>& newTeFlow) {
  const std::shared_ptr<TeFlowEntry> oldTeFlow = teFlow_;
  // Entry disabled
  if (!teFlow_->getEnabled() && !newTeFlow->getEnabled()) {
    teFlow_ = newTeFlow;
    updateStats(oldTeFlow, newTeFlow);
    return;
  }

  // disabled to enabled
  if (!teFlow_->getEnabled() && newTeFlow->getEnabled()) {
    teFlow_ = newTeFlow;
    createTeFlowActions();
    updateStats(oldTeFlow, newTeFlow);
    installTeFlowEntry();
    return;
  }

  // enabled to disabled
  if (teFlow_->getEnabled() && !newTeFlow->getEnabled()) {
    removeTeFlowActions();
    redirectNexthop_ = nullptr;
    teFlow_ = newTeFlow;
    updateStats(oldTeFlow, newTeFlow);
    installTeFlowEntry();
    return;
  }

  // enabled to enabled
  removeTeFlowActions();
  teFlow_ = newTeFlow;
  createTeFlowActions();
  updateStats(oldTeFlow, newTeFlow);
  installTeFlowEntry();
}

BcmTeFlowEntry::BcmTeFlowEntry(
    BcmSwitch* hw,
    int gid,
    const std::shared_ptr<TeFlowEntry>& teFlow)
    : hw_(hw), gid_(gid), teFlow_(teFlow) {
  const auto& flow = teFlow_->getFlow();
  const auto& srcPort = flow->cref<ctrl_if_tags::srcPort>();
  const auto& dstPrefix = flow->cref<ctrl_if_tags::dstPrefix>();
  if (srcPort && dstPrefix) {
    const auto prefix = dstPrefix->toThrift();
    const auto destIp = toIPAddress(*prefix.ip());
    const auto warmBootCache = hw_->getWarmBootCache();
    auto warmbootItr = warmBootCache->findTeFlow(srcPort->toThrift(), destIp);
    if (warmbootItr != warmBootCache->teFlow2BcmTeFlowEntryHandle_end()) {
      handle_ = warmbootItr->second;
      setRedirectNexthop(hw_, redirectNexthop_, teFlow_);
      // check whether teflow in S/W and H/W in sync
      CHECK(BcmTeFlowEntry::isStateSame(hw_, gid_, handle_, teFlow_))
          << "Warmboot TeFlow doesn't match the one from H/W";
      // if stat is enabled it will be recovered
      if (isStatEnabled(teFlow_)) {
        createTeFlowStat(teFlow_);
      }
      warmBootCache->programmed(warmbootItr);
    } else {
      createTeFlowEntry();
    }
  }
}

BcmTeFlowEntry::~BcmTeFlowEntry() {
  // Delete the TeFlow stat
  deleteTeFlowStat(teFlow_);

  // Destroy the TeFlow entry
  auto rv = bcm_field_entry_destroy(hw_->getUnit(), handle_);
  bcmLogFatal(rv, hw_, "failed to destroy the TeFlow entry");
}

TeFlow BcmTeFlowEntry::getID() const {
  // THRIFT_COPY
  return teFlow_->getFlow()->toThrift();
}

bool BcmTeFlowEntry::isStateSame(
    const BcmSwitch* hw,
    int gid,
    BcmTeFlowEntryHandle handle,
    const std::shared_ptr<TeFlowEntry>& teFlow) {
  auto teFlowMsg = folly::to<std::string>(
      "Group=", gid, ", teflow=", teFlow->str(), ", handle=", handle);
  bool isSame = true;

  XLOG(DBG3) << "Verify the state of Teflow entry: " << teFlowMsg;
  // check src port
  const auto& flow = teFlow->getFlow();
  const auto& srcPort = flow->get<ctrl_if_tags::srcPort>();
  if (srcPort) {
    isSame &= isBcmPortQualFieldStateSame(
        hw->getUnit(),
        handle,
        teFlowMsg,
        "SrcPort",
        bcm_field_qualify_SrcPort_get,
        srcPort->toThrift(),
        false);
  }

  // check dst Ip
  const auto& dstPrefix = flow->get<ctrl_if_tags::dstPrefix>();
  if (dstPrefix) {
    bcm_ip6_t addr;
    bcm_ip6_t mask{};
    auto prefix = dstPrefix->toThrift();
    folly::IPAddress ipaddr = toIPAddress(*prefix.ip());
    if (!ipaddr.isV6()) {
      throw FbossError("only ipv6 addresses are supported for teflow");
    }
    facebook::fboss::ipToBcmIp6(ipaddr, &addr);
    isSame &= isBcmQualFieldStateSame(
        bcm_field_qualify_DstIp6_get,
        hw->getUnit(),
        handle,
        true,
        addr,
        mask,
        teFlowMsg,
        "Dst IP",
        false);
  }

  int enabled = teFlow->getEnabled() ? 1 : 0;
  // check egressId
  uint32_t egressId = 0, param1 = 0;
  auto rv = bcm_field_action_get(
      hw->getUnit(), handle, bcmFieldActionL3Switch, &egressId, &param1);
  if (rv == BCM_E_NOT_FOUND) {
    // If the entry is disabled there is no nexthop
    // hence egressId should not be found
    isSame &= ((!enabled) ? 1 : 0);
  } else {
    bcmCheckError(
        rv, teFlowMsg, " failed to get action=", bcmFieldActionL3Switch);
    std::shared_ptr<BcmMultiPathNextHop> redirectNexthop;
    setRedirectNexthop(hw, redirectNexthop, teFlow);
    auto expectedEgressId = getEgressId(hw, redirectNexthop);
    if (egressId != static_cast<uint32_t>(expectedEgressId)) {
      XLOG(ERR) << teFlowMsg
                << " redirect nexthop egressId mismatched. SW expected="
                << expectedEgressId << " HW value=" << egressId;
      isSame = false;
    }
  }

  // check flow stat
  auto teFlowStatHandle = BcmTeFlowStat::getAclStatHandleFromAttachedAcl(
      hw, getEMGroupID(gid), handle);
  if (teFlowStatHandle && isStatEnabled(teFlow)) {
    cfg::TrafficCounter counter;
    counter.name() = teFlow->getCounterID()->toThrift();
    counter.types() = {cfg::CounterType::BYTES};
    isSame &=
        ((BcmTeFlowStat::isStateSame(hw, (*teFlowStatHandle).first, counter))
             ? 1
             : 0);
  } else {
    isSame &= ((!teFlowStatHandle.has_value()) ? 1 : 0);
  }

  return isSame;
}

} // namespace facebook::fboss
