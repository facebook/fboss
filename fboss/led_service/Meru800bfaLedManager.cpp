// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Meru800bfaLedManager.h"
#include "fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/meru800bfa/Meru800bfaBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Meru800bfaLedManager ctor()
 *
 * Meru800bfaLedManager constructor will create the LedManager object for
 * Meru800bfa platform
 */
Meru800bfaLedManager::Meru800bfaLedManager() : BspLedManager() {
  init<Meru800bfaBspPlatformMapping, Meru800bfaPlatformMapping>();
  XLOG(INFO) << "Created Meru800bfa BSP LED Manager";
}

} // namespace facebook::fboss
