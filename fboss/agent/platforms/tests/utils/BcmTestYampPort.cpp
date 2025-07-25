/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/tests/utils/BcmTestYampPort.h"

#include "fboss/agent/platforms/tests/utils/BcmTestYampPlatform.h"

namespace facebook::fboss {

BcmTestYampPort::BcmTestYampPort(
    const PortID& id,
    BcmTestYampPlatform* platform)
    : BcmTestPort(id, platform) {}

} // namespace facebook::fboss
