/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiMacsecManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

namespace facebook::fboss {
void SaiSwitch::updateStatsImpl() {
  auto portsIter = concurrentIndices_->portSaiId2PortInfo.begin();
  while (portsIter != concurrentIndices_->portSaiId2PortInfo.end()) {
    {
      std::lock_guard<std::mutex> locked(saiSwitchMutex_);
      managerTable_->portManager().updateStats(
          portsIter->second.portID, false /*updateWatermarks*/);
    }
    ++portsIter;
  }
}
} // namespace facebook::fboss
