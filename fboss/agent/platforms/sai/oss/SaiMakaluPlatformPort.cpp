/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiMakaluPlatformPort.h"

namespace facebook::fboss {

void SaiMakaluPlatformPort::linkStatusChanged(
    bool /* up */,
    bool /* adminUp */) {}

void SaiMakaluPlatformPort::externalState(PortLedExternalState /* lfs */) {}

uint32_t SaiMakaluPlatformPort::getCurrentLedState() const {
  return static_cast<uint32_t>(currentLedState_);
}

} // namespace facebook::fboss
