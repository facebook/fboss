/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/MinipackPort.h"

namespace facebook { namespace fboss {
LaneSpeeds MinipackPort::supportedLaneSpeeds() const {
  LaneSpeeds speeds;
  return speeds;
}

void MinipackPort::prepareForGracefulExit() {}

void MinipackPort::linkStatusChanged(bool up, bool adminUp) {
  WedgePort::linkStatusChanged(up, adminUp);
}

const BcmPlatformPort::XPEs MinipackPort::getEgressXPEs() const {
  return {};
}

bool MinipackPort::shouldDisableFEC() const {
  return false;
}
}} // facebook::fboss
