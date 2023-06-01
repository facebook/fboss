/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestPort.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"

namespace facebook::fboss {

BcmTestPort::BcmTestPort(PortID id, BcmTestPlatform* platform)
    : BcmPlatformPort(id, platform) {}

void BcmTestPort::setBcmPort(BcmPort* port) {
  bcmPort_ = port;
}

void BcmTestPort::preDisable(bool /*temporary*/) {}

void BcmTestPort::postDisable(bool /*temporary*/) {}

void BcmTestPort::preEnable() {}

void BcmTestPort::postEnable() {}

bool BcmTestPort::isMediaPresent() {
  return false;
}

void BcmTestPort::linkStatusChanged(bool /*up*/, bool /*adminUp*/) {}

void BcmTestPort::externalState(PortLedExternalState /* unused */) {}

bool BcmTestPort::supportsTransceiver() const {
  return true;
}

void BcmTestPort::statusIndication(
    bool /*enabled*/,
    bool /*link*/,
    bool /*ingress*/,
    bool /*egress*/,
    bool /*discards*/,
    bool /*errors*/) {}

void BcmTestPort::prepareForGracefulExit() {}

folly::Future<TransceiverInfo> BcmTestPort::getFutureTransceiverInfo() const {
  if (auto transceiver =
          getPlatform()->getOverrideTransceiverInfo(getPortID())) {
    return transceiver.value();
  }
  throw FbossError("failed to get transceiver info for ", getPortID());
}

int BcmTestPort::numberOfLanes() const {
  auto profile = getBcmPort()->getCurrentProfile();
  return getPlatform()->getLaneCount(profile);
}

} // namespace facebook::fboss
