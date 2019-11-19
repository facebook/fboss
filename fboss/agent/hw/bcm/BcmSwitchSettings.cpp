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
}

void BcmSwitchSettings::enableL2LearningSoftware() {
  if (l2LearningMode_.has_value() &&
      l2LearningMode_.value() == cfg::L2LearningMode::SOFTWARE) {
    return;
  }

  enableL2LearningCallback();
  enablePendingEntriesOnUnknownSrcL2();
}

} // namespace fboss
} // namespace facebook
