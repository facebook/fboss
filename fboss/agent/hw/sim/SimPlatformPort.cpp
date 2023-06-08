/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sim/SimPlatformPort.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sim/SimPlatform.h"

namespace facebook::fboss {
SimPlatformPort::SimPlatformPort(PortID id, SimPlatform* platform)
    : PlatformPort(id, platform) {}

void SimPlatformPort::preDisable(bool /* temporary */) {}
void SimPlatformPort::postDisable(bool /* temporary */) {}
void SimPlatformPort::preEnable() {}
void SimPlatformPort::postEnable() {}
bool SimPlatformPort::isMediaPresent() {
  return true;
}
void SimPlatformPort::linkStatusChanged(bool /* up */, bool /* adminUp */) {}
bool SimPlatformPort::supportsTransceiver() const {
  return false;
}

void SimPlatformPort::statusIndication(
    bool /* enabled */,
    bool /* link */,
    bool /* ingress */,
    bool /* egress */,
    bool /* discards */,
    bool /* errors */) {}
void SimPlatformPort::prepareForGracefulExit() {}
bool SimPlatformPort::shouldDisableFEC() const {
  return true;
}

folly::Future<TransceiverInfo> SimPlatformPort::getFutureTransceiverInfo()
    const {
  if (auto transceiver =
          getPlatform()->getOverrideTransceiverInfo(getPortID())) {
    return transceiver.value();
  }
  throw FbossError("failed to get transceiver info for ", getPortID());
}

std::shared_ptr<TransceiverSpec> SimPlatformPort::getTransceiverSpec() const {
  throw FbossError("failed to get transceiver spec for ", getPortID());
}
} // namespace facebook::fboss
