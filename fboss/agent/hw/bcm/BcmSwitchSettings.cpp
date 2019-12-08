/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmSwitchSettings.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmError.h"

namespace facebook {
namespace fboss {

void BcmSwitchSettings::setL2LearningMode(cfg::L2LearningMode l2LearningMode) {
  if (l2LearningMode == cfg::L2LearningMode::HARDWARE) {
    enableL2LearningHardware();
  } else if (l2LearningMode == cfg::L2LearningMode::SOFTWARE) {
    enableL2LearningSoftware();
  } else {
    throw FbossError(
        "Invalid L2LearningMode, only SOFTWARE and HARDWRE modes are supported: ",
        static_cast<int>(l2LearningMode));
  }

  l2LearningMode_ = l2LearningMode;
}

void BcmSwitchSettings::enableL2LearningHardware() {
  if (l2LearningMode_.has_value() &&
      l2LearningMode_.value() == cfg::L2LearningMode::HARDWARE) {
    return;
  }

  disableL2LearningCallback();
  disablePendingEntriesOnUnknownSrcL2();

  /*
   * For every L2 entry previously learned in SOFTWARE mode, remove it from
   * SwitchState's MAC Table.
   * This is achieved by traversing L2 table, and generating L2 table update
   * DELETE callback for each entry in L2 table.
   *
   * At this stage, L2 table update callbacks are disabled, so we don't need to
   * worry about more entries getting added into SwitchState's MAC Table.
   *
   * However, this results into BcmSwitch::processMacTableChanges receiving a
   * stateDelta with all L2 addresses removed. This would cause unknown unicast
   * flood till the L2 entries are relearned. To avoid,
   * BcmSwitch::processMacTableChanges is no-op when in
   * l2LearningMode::HARDWARE>
   */
  auto rv =
      opennsl_l2_traverse(hw_->getUnit(), BcmSwitch::deleteL2TableCb, hw_);
  bcmCheckError(rv, "opennsl_l2_traverse failed");
}

void BcmSwitchSettings::enableL2LearningSoftware() {
  if (l2LearningMode_.has_value() &&
      l2LearningMode_.value() == cfg::L2LearningMode::SOFTWARE) {
    return;
  }

  enableL2LearningCallback();
  enablePendingEntriesOnUnknownSrcL2();

  /*
   * For every L2 entry previously learned in HARDWARE mode, populate
   * SwitchState's MAC Table.
   * This is achieved by traversing L2 table, and generating L2 table update
   * ADD callback for each entry in L2 table.
   *
   * We don't need to worry about MACs learned after this traverse as L2 table
   * update callbacks are already enabled. However, this also means that we
   * will end up delivering duplicate callbacks for L2 entries learned in the
   * time window between enabling L2 table update callback and traversal. This
   * is fine given how our implementation handles it: a call to add macEntry
   * that exists is a no-op.
   */
  auto rv = opennsl_l2_traverse(hw_->getUnit(), BcmSwitch::addL2TableCb, hw_);
  bcmCheckError(rv, "opennsl_l2_traverse failed");
}

} // namespace fboss
} // namespace facebook
