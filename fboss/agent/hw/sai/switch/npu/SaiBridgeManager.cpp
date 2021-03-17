/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

void SaiBridgeManager::setL2LearningMode(
    std::optional<cfg::L2LearningMode> l2LearningMode) {
  if (l2LearningMode) {
    fdbLearningMode_ = getFdbLearningMode(l2LearningMode.value());
  }
  XLOG(INFO) << "FDB learning mode set to "
             << (getL2LearningMode() == cfg::L2LearningMode::HARDWARE
                     ? "hardware"
                     : "software");
}
} // namespace facebook::fboss
