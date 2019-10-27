/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/test_platforms/BcmTestPort.h"

#include "fboss/agent/platforms/test_platforms/BcmTestPlatform.h"

namespace facebook {
namespace fboss {

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

void BcmTestPort::linkSpeedChanged(const cfg::PortSpeed& /* unused */) {}

void BcmTestPort::externalState(BcmPlatformPort::ExternalState /* unused */) {}

folly::Future<TransmitterTechnology> BcmTestPort::getTransmitterTech(
    folly::EventBase* /*evb*/) const {
  return folly::makeFuture<TransmitterTechnology>(
      TransmitterTechnology::UNKNOWN);
}

folly::Future<folly::Optional<TxSettings>> BcmTestPort::getTxSettings(
    folly::EventBase* /*evb*/) const {
  return folly::none;
}

folly::Optional<TransceiverID> BcmTestPort::getTransceiverID() const {
  return folly::none;
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

} // namespace fboss
} // namespace facebook
