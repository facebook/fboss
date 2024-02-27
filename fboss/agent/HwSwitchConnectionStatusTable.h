/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <memory>
#include <string>

#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwSwitch;

class HwSwitchConnectionStatusTable {
 public:
  explicit HwSwitchConnectionStatusTable(SwSwitch* sw) : sw_(sw) {}
  void connected(SwitchID switchId);
  bool disconnected(SwitchID switchId);
  bool waitUntilHwSwitchConnected();
  void cancelWait();
  int getConnectionStatus(SwitchID switchId);

 private:
  std::set<SwitchID> connectedSwitches_;
  std::condition_variable hwSwitchConnectedCV_;
  std::mutex hwSwitchConnectedMutex_;
  bool connectionWaitCancelled_{false};
  SwSwitch* sw_;
};
} // namespace facebook::fboss
