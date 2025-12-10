// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Tahansb800bcLedManager.h"
#include "fboss/agent/platforms/common/tahansb800bc/Tahansb800bcPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/tahansb800bc/Tahansb800bcBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Tahansb800bcLedManager ctor()
 *
 * Tahansb800bcLedManager constructor will create the LedManager object for
 * Tahansb800bc platform
 */
Tahansb800bcLedManager::Tahansb800bcLedManager() : BspLedManager() {
  init<Tahansb800bcBspPlatformMapping, Tahansb800bcPlatformMapping>();
  XLOG(INFO) << "Created Tahansb800bc BSP LED Manager";
}

} // namespace facebook::fboss
