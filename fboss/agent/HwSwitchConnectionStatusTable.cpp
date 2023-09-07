/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/HwSwitchConnectionStatusTable.h"

namespace facebook::fboss {
void HwSwitchConnectionStatusTable::connected(SwitchID switchId) {
  bool isFirstConnection{false};
  {
    std::unique_lock<std::mutex> lk(hwSwitchConnectedMutex_);
    if (connectedSwitches_.empty()) {
      isFirstConnection = true;
    }
    connectedSwitches_.insert(switchId);
  }
  if (isFirstConnection) {
    // notify the waiting thread
    hwSwitchConnectedCV_.notify_one();
  }
}

void HwSwitchConnectionStatusTable::disconnected(SwitchID switchId) {
  std::unique_lock<std::mutex> lk(hwSwitchConnectedMutex_);
  connectedSwitches_.erase(switchId);
}

bool HwSwitchConnectionStatusTable::waitUntilHwSwitchConnected() {
  std::unique_lock<std::mutex> lk(hwSwitchConnectedMutex_);
  if (!connectedSwitches_.empty()) {
    return true;
  }
  hwSwitchConnectedCV_.wait(lk, [this] {
    return !connectedSwitches_.empty() || connectionWaitCancelled_;
  });
  return !connectionWaitCancelled_;
}

void HwSwitchConnectionStatusTable::cancelWait() {
  {
    std::unique_lock<std::mutex> lk(hwSwitchConnectedMutex_);
    connectionWaitCancelled_ = true;
  }
  hwSwitchConnectedCV_.notify_one();
}
} // namespace facebook::fboss
