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
    // Release the TeFlows before deleting teflow group and hint id.
    releaseTeFlows();
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

} // namespace facebook::fboss
