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

} // namespace facebook::fboss
