/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/FakeBcmTestPort.h"

#include "fboss/agent/platforms/tests/utils/FakeBcmTestPlatform.h"

namespace facebook::fboss {

FakeBcmTestPort::FakeBcmTestPort(PortID id, FakeBcmTestPlatform* platform)
    : BcmTestPort(id, platform) {}

} // namespace facebook::fboss
