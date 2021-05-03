/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiBcmWedge400PlatformPort.h"
#include "fboss/agent/platforms/common/utils/Wedge400LedUtils.h"

namespace facebook::fboss {

void SaiBcmWedge400PlatformPort::linkStatusChanged(
    bool /*up*/,
    bool /*adminUp*/) {
  // TODO(skhare)
}

void SaiBcmWedge400PlatformPort::externalState(PortLedExternalState /*lfs*/) {
  // TODO(skhare)
}

} // namespace facebook::fboss
