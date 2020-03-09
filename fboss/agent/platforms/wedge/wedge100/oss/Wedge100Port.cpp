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

#include <folly/logging/xlog.h>

namespace facebook::fboss {

// stubbed out
void Wedge100Port::linkStatusChanged(bool up, bool adminUp) {
  WedgePort::linkStatusChanged(up, adminUp);
}

void Wedge100Port::externalState(PortLedExternalState lfs) {
  XLOG(DBG1) << lfs;
}

void Wedge100Port::prepareForGracefulExit() {}

} // namespace facebook::fboss
