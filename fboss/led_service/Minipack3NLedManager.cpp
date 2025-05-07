// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Minipack3NLedManager.h"
#include "fboss/agent/platforms/common/minipack3n/Minipack3NPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/minipack3n/Minipack3NBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Minipack3NLedManager ctor()
 *
 * Minipack3NLedManager constructor will create the LedManager object for
 * Minipack3N platform
 */
Minipack3NLedManager::Minipack3NLedManager() : BspLedManager() {
  init<Minipack3NBspPlatformMapping, Minipack3NPlatformMapping>();
  XLOG(INFO) << "Created Minipack3N BSP LED Manager";
}

} // namespace facebook::fboss
