/*
 *  Copyright (c) 2023-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiMorgan800ccPlatformPort.h"

namespace facebook::fboss {

void SaiMorgan800ccPlatformPort::linkStatusChanged(
    bool /* up */,
    bool /* adminUp */) {}

void SaiMorgan800ccPlatformPort::externalState(PortLedExternalState /* lfs */) {
}

} // namespace facebook::fboss
