/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/wedge100/Wedge100Port.h"

#include "fboss/agent/platforms/wedge/wedge100/Wedge100Platform.h"

namespace facebook::fboss {

Wedge100Port::Wedge100Port(PortID id, Wedge100Platform* platform)
    : WedgePort(id, platform) {}

bool Wedge100Port::isTop() {
  return Wedge100LedUtils::isTop(getTransceiverID());
}

} // namespace facebook::fboss
