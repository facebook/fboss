/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/SplitAgentHwSwitchCallbackHandler.h"

namespace facebook::fboss {
void SplitAgentHwSwitchCallbackHandler::packetReceived(
    std::unique_ptr<RxPacket> /* pkt */) noexcept {
  // TODO - Add handler
}

void SplitAgentHwSwitchCallbackHandler::linkStateChanged(
    PortID /* port */,
    bool /* up */,
    std::optional<phy::LinkFaultStatus> /* iPhyFaultStatus */) {
  // TODO - Add handler
}

void SplitAgentHwSwitchCallbackHandler::l2LearningUpdateReceived(
    L2Entry /* l2Entry */,
    L2EntryUpdateType /* l2EntryUpdateType */) {
  // TODO - Add handler
}

void SplitAgentHwSwitchCallbackHandler::exitFatal() const noexcept {
  // TODO - Add handler
}

void SplitAgentHwSwitchCallbackHandler::pfcWatchdogStateChanged(
    const PortID& /* port */,
    const bool /* deadlock */) {
  // TODO - Add handler
}

void SplitAgentHwSwitchCallbackHandler::registerStateObserver(
    StateObserver* /* observer */,
    const std::string& /* name */) {
  // TODO - Add handler
}

void SplitAgentHwSwitchCallbackHandler::unregisterStateObserver(
    StateObserver* /* observer */) {
  // TODO - Add handler
}

} // namespace facebook::fboss
