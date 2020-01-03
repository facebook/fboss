/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SwSwitch.h"

#include <folly/Format.h>
#include <folly/Range.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

void SwSwitch::publishInitTimes(std::string /*name*/, const float& /*time*/) {}

void SwSwitch::updateRouteStats() {}
void SwSwitch::updatePortInfo() {}

void SwSwitch::publishSwitchInfo(struct HwInitResult /*hwInitRet*/) {}

void SwSwitch::logLinkStateEvent(PortID port, bool up) {
  std::string logMsg = folly::sformat(
      "LinkState: Port {0} {1}", (uint16_t)port, (up ? "Up" : "Down"));
  XLOG(DBG2) << logMsg;
}

void SwSwitch::logSwitchRunStateChange(
    SwitchRunState oldState,
    SwitchRunState newState) {
  std::string logMsg = folly::sformat(
      "SwitchRunState changed from {} to {}",
      switchRunStateStr(oldState),
      switchRunStateStr(newState));
  XLOG(DBG2) << logMsg;
}

} // namespace facebook::fboss
