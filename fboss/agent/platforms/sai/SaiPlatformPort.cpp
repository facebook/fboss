/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiPlatformPort.h"

namespace facebook {
namespace fboss {

PortID SaiPlatformPort::getPortID() const {
  return PortID(42);
}
void SaiPlatformPort::preDisable(bool /* temporary */) {}
void SaiPlatformPort::postDisable(bool /* temporary */) {}
void SaiPlatformPort::preEnable() {}
void SaiPlatformPort::postEnable() {}
bool SaiPlatformPort::isMediaPresent() {
  return true;
}
void SaiPlatformPort::linkStatusChanged(bool /* up */, bool /* adminUp */) {}
void SaiPlatformPort::linkSpeedChanged(const cfg::PortSpeed& /* speed */) {}
bool SaiPlatformPort::supportsTransceiver() const {
  return false;
}
folly::Optional<TransceiverID> SaiPlatformPort::getTransceiverID() const {
  return folly::none;
}
void SaiPlatformPort::statusIndication(
    bool /* enabled */,
    bool /* link */,
    bool /* ingress */,
    bool /* egress */,
    bool /* discards */,
    bool /* errors */) {}
void SaiPlatformPort::prepareForGracefulExit() {}
bool SaiPlatformPort::shouldDisableFEC() const {
  return true;
}

} // namespace fboss
} // namespace facebook
