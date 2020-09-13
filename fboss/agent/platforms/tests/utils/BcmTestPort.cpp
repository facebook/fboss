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

folly::Future<TransmitterTechnology> BcmTestPort::getTransmitterTech(
    folly::EventBase* /*evb*/) const {
  auto entry = getPlatformPortEntry();
  if (entry && entry->mapping_ref()->name_ref()->find("fab") == 0) {
    return folly::makeFuture<TransmitterTechnology>(
        TransmitterTechnology::COPPER);
  }
  return folly::makeFuture<TransmitterTechnology>(
      TransmitterTechnology::UNKNOWN);
}

std::optional<TransceiverID> BcmTestPort::getTransceiverID() const {
  return std::nullopt;
}

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

folly::Future<TransceiverInfo> BcmTestPort::getTransceiverInfo() const {
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
