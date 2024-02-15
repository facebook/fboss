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

#include <folly/logging/xlog.h>

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

bool HwSwitchConnectionStatusTable::disconnected(SwitchID switchId) {
  std::unique_lock<std::mutex> lk(hwSwitchConnectedMutex_);
  // Switch was not connected. This can happen if switch sends a
  // graceful restart notification and later thrift stream detects
  // a connection loss and notifies the disconnect.
  if (!connectedSwitches_.erase(switchId)) {
    return false;
  }
  // If there are no more active connections to HwSwitch, we cannot
  // apply any new state updates. Hence exit the SwSwitch.
  // In normal shutdown sequence, we exit SwSwitch first before shutting
  // down HwSwitch. Hence this condition can happen only if all HwSwitches crash
  if (connectedSwitches_.empty()) {
    XLOG(FATAL) << "No active HwSwitch connections";
  }
  return true;
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
