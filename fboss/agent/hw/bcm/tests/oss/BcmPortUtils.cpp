/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmPortUtils.h"

namespace facebook::fboss::utility {
void assertPortLoopbackMode(
    int /*unit*/,
    PortID /*port*/,
    int /*expectedLoopbackMode*/) {
  CHECK(false) << "Loopback mode not supported";
}

} // namespace facebook::fboss::utility
