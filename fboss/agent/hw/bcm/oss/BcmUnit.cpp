/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmUnit.h"

#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

extern "C" {
#include <bcm/init.h>
#include <bcm/switch.h>
} // extern "C"

using facebook::fboss::BcmAPI;
using folly::StringPiece;

namespace facebook::fboss {

int BcmUnit::createHwUnit() {
  // For now we assume that the unit number is 0. This will be changed once
  // bcm exposes interfaces to determine which units are on the system
  return 0;
}

BcmUnit::~BcmUnit() {
  // TODO(skhare) fix by using BcmUnit.cpp that uses BCM API
}

void BcmUnit::attach(bool warmBoot) {
  // TODO(skhare) fix by using BcmUnit.cpp that uses BCM API
}

void BcmUnit::rawRegisterWrite(
    uint16_t /*phyID*/,
    uint8_t /*reg*/,
    uint16_t /*data*/) {
  // stubbed out
}

void BcmUnit::detachAndCleanupSDKUnit() {
  // not supported
}

} // namespace facebook::fboss
