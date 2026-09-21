// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Icecube800bcLedManager.h"
#include "fboss/agent/platforms/common/icecube800bc/Icecube800bcPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/icecube800bc/Icecube800bcBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Icecube800bcLedManager ctor()
 *
 * Icecube800bcLedManager constructor will create the LedManager object for
 * icecube800bc platform
 */
Icecube800bcLedManager::Icecube800bcLedManager() : BspLedManager() {
  init<Icecube800bcBspPlatformMapping, Icecube800bcPlatformMapping>();
  XLOG(INFO) << "Created Icecube800bc BSP LED Manager";
}

} // namespace facebook::fboss
