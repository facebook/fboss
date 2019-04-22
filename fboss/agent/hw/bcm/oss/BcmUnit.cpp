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
#include <opennsl/init.h>
#include <opennsl/switch.h>
#include <sal/driver.h>
} // extern "C"

using folly::StringPiece;
using facebook::fboss::BcmAPI;

namespace facebook { namespace fboss {

namespace {
constexpr auto wbEnvVar = "SOC_BOOT_FLAGS";
constexpr auto wbFlag = "0x200000";
}

int BcmUnit::createHwUnit()  {
  // For now we assume that the unit number is 0. This will be changed once
  // opennsl exposes interfaces to determine which units are on the system
  return 0;
}

BcmUnit::~BcmUnit() {
  if (attached_.load(std::memory_order_acquire)) {
    auto rv = opennsl_detach(unit_);
    CHECK(OPENNSL_SUCCESS(rv)) <<
        "failed to detach BCM unit " << unit_ << ": " << opennsl_errmsg(rv);
  }
  // Unregister ourselves from BcmAPI.
  BcmAPI::unitDestroyed(this);
}

void BcmUnit::attach(bool warmBoot) {
  if (attached_.load(std::memory_order_acquire)) {
    throw FbossError("unit ", unit_, " already initialized");
  }
  // FIXME: Pass in initial config and warm boot flags
  opennsl_driver_init(nullptr);

  attached_.store(true, std::memory_order_release);
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
}} // facebook::fboss
