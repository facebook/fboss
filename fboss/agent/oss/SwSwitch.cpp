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
#include <folly/Range.h>
#include <folly/ThreadName.h>

namespace facebook { namespace fboss {

void SwSwitch::initThread(folly::StringPiece name) {
  // We need a null-terminated string to pass to folly::setThreadName().
  // The pthread name can be at most 15 bytes long, so truncate it if necessary
  // when creating the string.
  size_t pthreadLength = std::min(name.size(), (size_t)15);
  char pthreadName[pthreadLength + 1];
  memcpy(pthreadName, name.begin(), pthreadLength);
  pthreadName[pthreadLength] = '\0';
  folly::setThreadName(pthreadName);
}

void SwSwitch::publishInitTimes(std::string name, const float& time) {}

void SwSwitch::publishStats() {}

void SwSwitch::publishSwitchInfo(struct HwInitResult hwInitRet) {}

void SwSwitch::logLinkStateEvent(PortID port, bool up) {
  std::string logMsg = folly::sformat("LinkState: Port {0} {1}",
                                      (uint16_t)port, (up ? "Up" : "Down"));
  VLOG(2) << logMsg;
}

void SwSwitch::logSwitchRunStateChange(
    const SwitchRunState& oldState,
    const SwitchRunState& newState) {
  std::string logMsg = folly::sformat(
      "SwitchRunState changed from {} to {}",
      switchRunStateStr(oldState),
      switchRunStateStr(newState));
  VLOG(2) << logMsg;
}
}} // facebook::fboss
