/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiBcmIcetea800bcPlatformPort.h"

namespace facebook::fboss {

void SaiBcmIcetea800bcPlatformPort::linkStatusChanged(
    bool /*up*/,
    bool /*adminUp*/) {
  // TODO: set led color
}

void SaiBcmIcetea800bcPlatformPort::externalState(
    PortLedExternalState /*lfs*/) {
  // TODO: set led color
}

} // namespace facebook::fboss
