/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/wedge40/Wedge40Port.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

void Wedge40Port::prepareForGracefulExit() {}

void Wedge40Port::linkStatusChanged(bool up, bool adminUp) {
  WedgePort::linkStatusChanged(up, adminUp);
}

void Wedge40Port::externalState(PortLedExternalState lfs) {
  XLOG(DBG1) << lfs;
}

} // namespace facebook::fboss
