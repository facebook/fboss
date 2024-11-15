/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
extern "C" {
#include <bcm/flexctr.h>
#include <bcm/l3.h>
}

#include <folly/logging/xlog.h>
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmRouteCounter.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

namespace facebook::fboss {

folly::dynamic BcmRouteCounterID::toFollyDynamic() const {
  folly::dynamic counterObj = folly::dynamic::object;
  counterObj[kCounterIndex] = id_;
  counterObj[kCounterOffset] = offset_;
  return counterObj;
}

BcmRouteCounterID BcmRouteCounterID::fromFollyDynamic(
    const folly::dynamic& json) {
  return BcmRouteCounterID(
      json[kCounterIndex].asInt(), json[kCounterOffset].asInt());
}

BcmRouteCounterBase::BcmRouteCounterBase(
    BcmSwitch* hw,
    RouteCounterID counterID,
    int modeId)
    : hw_(hw) {
  if (!hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::ROUTE_COUNTERS)) {
    throw FbossError("Route counters are not supported on this platform");
  }
}

BcmRouteCounter::BcmRouteCounter(
    BcmSwitch* hw,
    RouteCounterID counterID,
    int modeId)
    : BcmRouteCounterBase(hw, counterID, modeId) {
  uint32 numCounters;
  const auto counterFromWBCache =
      hw_->getWarmBootCache()->findRouteCounterID(counterID);
  if (counterFromWBCache != hw_->getWarmBootCache()->RouteCounterIDMapEnd()) {
    hwCounterId_ = counterFromWBCache->second;
    XLOG(DBG2) << "Restored route counter id " << counterID << " hwId "
               << hwCounterId_.str();
    hw_->getWarmBootCache()->programmed(counterFromWBCache);
  } else {
    uint32_t hwIndex{0};
    /* Attach stat to customized group mode */
    auto rc = bcm_stat_custom_group_create(
        hw_->getUnit(),
        modeId,
        bcmStatObjectIngL3Route,
        &hwIndex,
        &numCounters);
    bcmCheckError(rc, "failed to create bcm stat custom group ");
    hwCounterId_ = BcmRouteCounterID(hwIndex, 0);
    XLOG(DBG2) << "Allocated route counter " << hwCounterId_.str();
  }
  hw_->getStatUpdater()->toBeAddedRouteCounter(hwCounterId_, counterID);
}

BcmRouteCounter::~BcmRouteCounter() {
  XLOG(DBG2) << "Destroying route counter " << hwCounterId_.str();
  auto rc = bcm_stat_group_destroy(hw_->getUnit(), hwCounterId_.getHwId());
  bcmLogFatal(rc, hw_, "failed to destroy counter ", hwCounterId_.str());
  hw_->getStatUpdater()->toBeRemovedRouteCounter(hwCounterId_);
}

BcmRouteCounterID BcmRouteCounter::getHwCounterID() const {
  return hwCounterId_;
}

BcmRouteFlexCounter::BcmRouteFlexCounter(
    BcmSwitch* hw,
    RouteCounterID counterID,
    std::shared_ptr<BcmFlexCounterAction> counterAction)
    : BcmRouteCounterBase(hw, counterID, counterAction->getActionId()),
      counterAction_(counterAction) {
  const auto counterFromWBCache =
      hw_->getWarmBootCache()->findRouteCounterID(counterID);
  if (counterFromWBCache != hw_->getWarmBootCache()->RouteCounterIDMapEnd()) {
    hwCounterId_ = counterFromWBCache->second;
    XLOG(DBG2) << "Restored route counter " << counterID << " hwId "
               << hwCounterId_.str();
    hw_->getWarmBootCache()->programmed(counterFromWBCache);
    counterAction->setOffset(hwCounterId_.getHwOffset());
  } else {
    hwCounterId_ = BcmRouteCounterID(
        counterAction->getActionId(), counterAction->allocateOffset());
    XLOG(DBG2) << "Allocated route counter " << hwCounterId_.str();
  }
  hw_->getStatUpdater()->toBeAddedRouteCounter(hwCounterId_, counterID);
}

BcmRouteCounterID BcmRouteFlexCounter::getHwCounterID() const {
  return hwCounterId_;
}
BcmRouteFlexCounter::~BcmRouteFlexCounter() {
  XLOG(DBG2) << "Destroying route counter " << hwCounterId_.str();
  counterAction_->clearOffset(hwCounterId_.getHwOffset());
  hw_->getStatUpdater()->toBeRemovedRouteCounter(hwCounterId_);
}

uint32_t BcmRouteCounterTable::createStatGroupModeId() {
  uint32_t totalCounters = 1;
  int numSelectors = 1;
  bcm_stat_group_mode_attr_selector_t attrSelectors[1];
  uint32 modeId = 0;

  /* Selector for non-drop packets  */
  bcm_stat_group_mode_attr_selector_t_init(&attrSelectors[0]);
  attrSelectors[0].counter_offset = 0;
  attrSelectors[0].attr = bcmStatGroupModeAttrDrop;
  attrSelectors[0].attr_value = 0;

  auto rc = bcm_stat_group_mode_id_create(
      hw_->getUnit(),
      BCM_STAT_GROUP_MODE_INGRESS,
      totalCounters,
      numSelectors,
      attrSelectors,
      &modeId);
  bcmCheckError(rc, "failed to create bcm stat group mode id");
  return modeId;
}

// used for testing purpose
std::optional<BcmRouteCounterID> BcmRouteCounterTable::getHwCounterID(
    std::optional<RouteCounterID> counterID) const {
  if (!counterID.has_value()) {
    return std::nullopt;
  }
  auto hwCounter = counterIDs_.get(*counterID);
  return hwCounter
      ? std::optional<BcmRouteCounterID>(hwCounter->getHwCounterID())
      : std::nullopt;
}

std::shared_ptr<BcmRouteCounterBase>
BcmRouteCounterTable::referenceOrEmplaceCounterID(RouteCounterID id) {
  if (globalIngressModeId_ == STAT_MODEID_INVALID) {
    const auto idFromWBCache = hw_->getWarmBootCache()->getRouteCounterModeId();
    if (idFromWBCache != STAT_MODEID_INVALID) {
      globalIngressModeId_ = idFromWBCache;
      XLOG(DBG2) << "Restored route counter global mode id "
                 << globalIngressModeId_ << " from warmboot cache";
    } else {
      globalIngressModeId_ = createStatGroupModeId();
      XLOG(DBG2) << "Allocated route counter global mode id "
                 << globalIngressModeId_;
    }
  }
  std::shared_ptr<BcmRouteCounterBase> counterRef =
      counterIDs_.refOrEmplace(id, hw_, id, globalIngressModeId_).first;
  if (counterIDs_.size() > maxRouteCounterIDs_) {
    XLOG(ERR) << "RouteCounterIDs in use " << counterIDs_.size()
              << " exceed max count " << maxRouteCounterIDs_;
    // throw Bcm full error so that overflow error handling kicks in
    throw BcmError(
        BCM_E_FULL,
        "RouteCounterIDs in use ",
        counterIDs_.size(),
        " exceed max count ",
        maxRouteCounterIDs_);
  }
  return counterRef;
}

folly::dynamic BcmRouteCounterTable::toFollyDynamic() const {
  folly::dynamic routeCounterIDs = folly::dynamic::object;
  for (const auto& entry : counterIDs_) {
    auto counter = entry.second.lock();
    // Temporarly store TH3 counter id in old format so that warmboot
    // downgrades to old image works. This can be removed once
    // code that understands new format is deployed everywhere
    routeCounterIDs[entry.first] = counter->getHwCounterID().getHwId();
  }
  folly::dynamic routeCounterInfo = folly::dynamic::object;
  routeCounterInfo[kRouteCounterIDs] = std::move(routeCounterIDs);
  int modeId = globalIngressModeId_;
  /*
    If global mode id is not valid, check whether warmboot cache
    has an entry that needs to be saved. This scenario can happen
    when there are two back to back warmboots and routes with counters
    are withdrawn in between.
  */
  if (modeId == STAT_MODEID_INVALID) {
    int idFromWBCache = hw_->getWarmBootCache()->getRouteCounterModeId();
    if (idFromWBCache != STAT_MODEID_INVALID) {
      modeId = idFromWBCache;
    }
  }
  routeCounterInfo[kGlobalModeId] = std::to_string(modeId);
  return routeCounterInfo;
}

BcmRouteCounterTable::~BcmRouteCounterTable() {
  if (globalIngressModeId_ != STAT_MODEID_INVALID) {
    XLOG(DBG2) << "Destroying stat group mode id " << globalIngressModeId_;
    auto rc =
        bcm_stat_group_mode_id_destroy(hw_->getUnit(), globalIngressModeId_);
    bcmLogFatal(rc, hw_, "failed to destroy bcm stat group mode id");
  }
}

std::shared_ptr<BcmRouteCounterBase>
BcmRouteFlexCounterTable::referenceOrEmplaceCounterID(RouteCounterID id) {
  if (!v6FlexCounterAction->getActionId()) {
    const auto idFromWBCache =
        hw_->getWarmBootCache()->getRouteCounterV6FlexActionId();
    if (idFromWBCache) {
      v6FlexCounterAction->setActionId(idFromWBCache);
      XLOG(DBG2) << "Restored flex route counter action id " << idFromWBCache
                 << " from warmboot cache";
    } else {
      v6FlexCounterAction->createFlexCounterAction();
      XLOG(DBG2) << "Allocated route flex counter action id "
                 << v6FlexCounterAction->getActionId();
    }
  }
  std::shared_ptr<BcmRouteCounterBase> counterRef =
      counterIDs_.refOrEmplace(id, hw_, id, v6FlexCounterAction).first;
  if (counterIDs_.size() > maxRouteCounterIDs_) {
    XLOG(ERR) << "RouteCounterIDs in use " << counterIDs_.size()
              << " exceed max count " << maxRouteCounterIDs_;
    // throw Bcm full error so that overflow error handling kicks in
    throw BcmError(
        BCM_E_FULL,
        "RouteCounterIDs in use ",
        counterIDs_.size(),
        " exceed max count ",
        maxRouteCounterIDs_);
  }
  return counterRef;
}

// used for testing purpose
std::optional<BcmRouteCounterID> BcmRouteFlexCounterTable::getHwCounterID(
    std::optional<RouteCounterID> counterID) const {
  if (!counterID.has_value()) {
    return std::nullopt;
  }
  auto hwCounter = counterIDs_.get(*counterID);
  return hwCounter
      ? std::optional<BcmRouteCounterID>(hwCounter->getHwCounterID())
      : std::nullopt;
}

folly::dynamic BcmRouteFlexCounterTable::toFollyDynamic() const {
  folly::dynamic routeCounterIDs = folly::dynamic::object;
  for (const auto& entry : counterIDs_) {
    auto counter = entry.second.lock();
    routeCounterIDs[entry.first] = counter->getHwCounterID().toFollyDynamic();
  }
  folly::dynamic routeCounterInfo = folly::dynamic::object;
  routeCounterInfo[kRouteCounterIDs] = std::move(routeCounterIDs);

  auto actionId = v6FlexCounterAction->getActionId();
  if (!actionId) {
    // Check whether warmboot cache has a valid action id
    auto idFromWBCache =
        hw_->getWarmBootCache()->getRouteCounterV6FlexActionId();
    if (idFromWBCache) {
      actionId = idFromWBCache;
    }
  }
  routeCounterInfo[kFlexCounterActionV6] = std::to_string(actionId);
  return routeCounterInfo;
}

BcmRouteFlexCounterTable::BcmRouteFlexCounterTable(BcmSwitch* hw)
    : BcmRouteCounterTableBase(hw) {
  v6FlexCounterAction = std::make_shared<BcmFlexCounterAction>(hw);
}

/*
 * On tomahawk3 devices, route counters are allocated dynamically
 * and attached to routes. On tomahawk4 devices, there cannot be
 * multiple route counter actions. Instead all V6 routes share a
 * counter action object. The counter action object has multiple
 * offsets which are preallocated. Individual route counters point
 * to one of the offset in counter action. This creates multiple
 * limitation on TH4.
 * 1) We have to preallocate max counters
 * 2) If max counters change, the following sequence is needed
 *      - detach all routes pointing to existing counter action
 *      - deallocate counter action
 *      - create new counter action with max counter
 *      - reattach all counters
 * 3) V6 and V4 prefixes cannot share counters
 * Due to this limitation, we are pre allocating kMaxFlexRouteCounters(1k)
 * counters regardless of maxRouteCounterID value. This allows us to
 * avoid complexity of maintaing counter to route mapping.
 * If kMaxFlexRouteCounters were to change in future, the route counters
 * needs to be withdrawn and readded from application.
 */
void BcmFlexCounterAction::createFlexCounterAction() {
  bcm_flexctr_action_t action;
  bcm_flexctr_index_operation_t* index_op;
  bcm_flexctr_value_operation_t* value_a_op;
  bcm_flexctr_value_operation_t* value_b_op;

  bcm_flexctr_action_t_init(&action);
  action.flags = 0;
  action.source = bcmFlexctrSourceL3Route;
  action.mode = bcmFlexctrCounterModeNormal;
  action.index_num = kMaxFlexRouteCounters; /* Number of required counters */
  index_op = &action.index_operation;
  index_op->object[0] = bcmFlexctrObjectStaticIngAlpmDstLookup;
  index_op->mask_size[0] = 7;
  index_op->shift[0] = 0;
  index_op->object[1] = bcmFlexctrObjectConstZero;
  index_op->mask_size[1] = 1;
  index_op->shift[1] = 0;

  /* Increase counter per packet. */
  value_a_op = &action.operation_a;
  value_a_op->select = bcmFlexctrValueSelectCounterValue;
  value_a_op->object[0] = bcmFlexctrObjectConstOne;
  value_a_op->mask_size[0] = 16;
  value_a_op->shift[0] = 0;
  value_a_op->object[1] = bcmFlexctrObjectConstZero;
  value_a_op->mask_size[1] = 1;
  value_a_op->shift[1] = 0;
  value_a_op->type = bcmFlexctrValueOperationTypeInc;

  /* Increase counter per packet bytes. */
  value_b_op = &action.operation_b;
  value_b_op->select = bcmFlexctrValueSelectPacketLength;
  value_b_op->type = bcmFlexctrValueOperationTypeInc;

  auto rc = bcm_flexctr_action_create(hw_->getUnit(), 0, &action, &actionId_);

  bcmCheckError(rc, "failed to create bcm flex counter action");
}

BcmFlexCounterAction::~BcmFlexCounterAction() {
  if (actionId_) {
    auto rc = bcm_flexctr_action_destroy(hw_->getUnit(), actionId_);
    bcmLogFatal(rc, hw_, "failed to destroy flexctr action id ", actionId_);
  }
}

uint16_t BcmFlexCounterAction::allocateOffset() {
  for (auto i = 0; i < kMaxFlexRouteCounters; i++) {
    if (!offsetMap_.test(i)) {
      offsetMap_.set(i);
      return i;
    }
  }
  // throw Bcm full error so that overflow error handling kicks in
  throw BcmError(BCM_E_FULL, "Route Flexcounter offsets exhausted");
}

void BcmFlexCounterAction::clearOffset(uint16_t offset) {
  DCHECK_EQ(offsetMap_.test(offset), true);
  offsetMap_.reset(offset);
}

void BcmFlexCounterAction::setOffset(uint16_t offset) {
  DCHECK_EQ(offsetMap_.test(offset), false);
  offsetMap_.set(offset);
}
} // namespace facebook::fboss
