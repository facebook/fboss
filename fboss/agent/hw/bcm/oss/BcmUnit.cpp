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

using facebook::fboss::BcmAPI;
using folly::StringPiece;

namespace facebook {
namespace fboss {

int BcmUnit::createHwUnit() {
  // For now we assume that the unit number is 0. This will be changed once
  // opennsl exposes interfaces to determine which units are on the system
  return 0;
}

BcmUnit::~BcmUnit() {
  if (attached_.load(std::memory_order_acquire)) {
    auto rv = opennsl_driver_exit();
    CHECK(OPENNSL_SUCCESS(rv))
        << "failed to exit BCM unit " << unit_ << ": " << opennsl_errmsg(rv);
    if (warmBootHelper()->warmBootStateWritten()) {
      warmBootHelper()->setCanWarmBoot();
    }
    attached_.store(false, std::memory_order_release);
  }
  // Unregister ourselves from BcmAPI.
  BcmAPI::unitDestroyed(this);
}

void BcmUnit::attach(bool warmBoot) {
  if (attached_.load(std::memory_order_acquire)) {
    throw FbossError("unit ", unit_, " already initialized");
  }
  auto hwConfigFile = platform_->getHwConfigDumpFile();
  auto warmBootFile =
      static_cast<DiscBackedBcmWarmBootHelper*>(warmBootHelper())
          ->warmBootDataPath();
  constexpr unsigned int wbFlag = 0x200000;
  opennsl_init_t initParam = {
      .cfg_fname = const_cast<char*>(hwConfigFile.c_str()),
      .flags = warmBootHelper()->canWarmBoot() ? wbFlag : 0,
      .wb_fname = const_cast<char*>(warmBootFile.c_str()),
      .rmcfg_fname = nullptr,
      .cfg_post_fname = nullptr,
      .opennsl_flags = 0};
  auto rv = opennsl_driver_init(&initParam);
  bcmCheckError(rv, "Unable to init driver");

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
} // namespace fboss
} // namespace facebook
