/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTeFlowTable.h"
#include <fboss/agent/hw/bcm/BcmAclStat.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmExactMatchUtils.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include <folly/CppAttributes.h>

namespace facebook::fboss {

using namespace facebook::fboss::utility;

void BcmTeFlowTable::createTeFlowGroup() {
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::EXACT_MATCH)) {
    teFlowGroupId_ = hw_->getPlatform()->getAsic()->getDefaultTeFlowGroupID();
    initEMTable(hw_->getUnit(), teFlowGroupId_, hintId_);
  } else {
    throw FbossError("Enabling exact match is not supported on this platform");
  }
}

/*
 * Release all teflow, stat entries.
 * Should only be called when we are about to reset/destroy the teflow table
 */
void BcmTeFlowTable::releaseTeFlows() {
  teFlowEntryMap_.clear();
}

void BcmTeFlowTable::processAddedTeFlow(
    const int groupId,
    const std::shared_ptr<TeFlowEntry>& teFlow) {
  if (teFlowEntryMap_.find(teFlow->getFlow()) != teFlowEntryMap_.end()) {
    throw FbossError("TeFlow=", teFlow->str(), " already exists");
  }

  std::unique_ptr<BcmTeFlowEntry> bcmTeFlow =
      std::make_unique<BcmTeFlowEntry>(hw_, groupId, teFlow);
  const auto& entry =
      teFlowEntryMap_.emplace(teFlow->getFlow(), std::move(bcmTeFlow));
  if (!entry.second) {
    throw FbossError("Failed to add an existing TeFlow entry");
  }
}

void BcmTeFlowTable::processRemovedTeFlow(
    const std::shared_ptr<TeFlowEntry>& teFlow) {
  auto iter = teFlowEntryMap_.find(teFlow->getFlow());
  if (iter == teFlowEntryMap_.end()) {
    throw FbossError("Failed to erase an non-existing TeFlow=", teFlow->str());
  }
  teFlowEntryMap_.erase(iter);
}

BcmTeFlowEntry* FOLLY_NULLABLE
BcmTeFlowTable::getTeFlowIf(const std::shared_ptr<TeFlowEntry>& teFlow) const {
  auto iter = teFlowEntryMap_.find(teFlow->getFlow());
  if (iter == teFlowEntryMap_.end()) {
    return nullptr;
  }
  return iter->second.get();
}

BcmTeFlowEntry* BcmTeFlowTable::getTeFlow(
    const std::shared_ptr<TeFlowEntry>& teFlow) const {
  auto teFlowEntry = getTeFlowIf(teFlow);
  if (!teFlowEntry) {
    throw FbossError("BCM TeFlow does not exist");
  }
  return teFlowEntry;
}

void BcmTeFlowTable::processChangedTeFlow(
    const int groupId,
    const std::shared_ptr<TeFlowEntry>& oldTeFlow,
    const std::shared_ptr<TeFlowEntry>& newTeFlow) {
  // TODO - Modify the code for Make before break or
  // inplace update to prevent any traffic loss in this case
  processRemovedTeFlow(oldTeFlow);
  processAddedTeFlow(groupId, newTeFlow);
}

BcmTeFlowTable::~BcmTeFlowTable() {
  if (FLAGS_enable_exact_match) {
    if (teFlowGroupId_) {
      auto rv =
          bcm_field_group_destroy(hw_->getUnit(), getEMGroupID(teFlowGroupId_));
      bcmLogFatal(rv, hw_, "failed to delete field group id ", teFlowGroupId_);
      teFlowGroupId_ = 0;
    }
    if (hintId_) {
      auto rv = bcm_field_hints_destroy(hw_->getUnit(), hintId_);
      bcmLogFatal(rv, hw_, "failed to delete hint id ", hintId_);
      hintId_ = 0;
    }
  }
}

BcmTeFlowStat* BcmTeFlowTable::incRefOrCreateBcmTeFlowStat(
    const std::string& counterName,
    int gid) {
  auto teFlowStatItr = teFlowStatMap_.find(counterName);
  if (teFlowStatItr == teFlowStatMap_.end()) {
    auto counterTypes = {cfg::CounterType::BYTES};
    auto newStat = std::make_unique<BcmTeFlowStat>(
        hw_, getEMGroupID(gid), counterTypes, BcmTeFlowStatType::EM);
    auto stat = newStat.get();
    teFlowStatMap_.emplace(counterName, std::make_pair(std::move(newStat), 1));
    hw_->getStatUpdater()->toBeAddedTeFlowStat(
        stat->getHandle(), counterName, counterTypes);
    return stat;
  } else {
    teFlowStatItr->second.second++;
    return teFlowStatItr->second.first.get();
  }
}

BcmTeFlowStat* BcmTeFlowTable::incRefOrCreateBcmTeFlowStat(
    const std::string& counterName,
    BcmTeFlowStatHandle statHandle) {
  auto teFlowStatItr = teFlowStatMap_.find(counterName);
  if (teFlowStatItr == teFlowStatMap_.end()) {
    auto counterTypes = {cfg::CounterType::BYTES};
    auto newStat =
        std::make_unique<BcmTeFlowStat>(hw_, statHandle, BcmTeFlowStatType::EM);
    auto stat = newStat.get();
    teFlowStatMap_.emplace(counterName, std::make_pair(std::move(newStat), 1));
    hw_->getStatUpdater()->toBeAddedTeFlowStat(
        stat->getHandle(), counterName, counterTypes);
    return stat;
  } else {
    CHECK(statHandle == teFlowStatItr->second.first->getHandle());
    teFlowStatItr->second.second++;
    return teFlowStatItr->second.first.get();
  }
}

void BcmTeFlowTable::derefBcmTeFlowStat(const std::string& counterName) {
  auto teFlowStatItr = teFlowStatMap_.find(counterName);
  if (teFlowStatItr == teFlowStatMap_.end()) {
    throw FbossError(
        "Tried to delete a non-existent TE flow stat: ", counterName);
  }
  auto bcmTeFlowStat = teFlowStatItr->second.first.get();
  auto& teFlowStatRefCnt = teFlowStatItr->second.second;
  teFlowStatRefCnt--;
  if (!teFlowStatRefCnt) {
    utility::deleteCounter(
        utility::statNameFromCounterType(counterName, cfg::CounterType::BYTES));
    hw_->getStatUpdater()->toBeRemovedTeFlowStat(bcmTeFlowStat->getHandle());
    teFlowStatMap_.erase(teFlowStatItr);
  }
}

BcmTeFlowStat* FOLLY_NULLABLE
BcmTeFlowTable::getTeFlowStatIf(const std::string& name) const {
  auto iter = teFlowStatMap_.find(name);
  if (iter == teFlowStatMap_.end()) {
    return nullptr;
  } else {
    return iter->second.first.get();
  }
}

BcmTeFlowStat* BcmTeFlowTable::getTeFlowStat(const std::string& stat) const {
  auto bcmStat = getTeFlowStatIf(stat);
  if (!bcmStat) {
    throw FbossError("Stat: ", stat, " does not exist");
  }
  return bcmStat;
}

} // namespace facebook::fboss
