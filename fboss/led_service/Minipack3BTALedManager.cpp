// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Minipack3BTALedManager.h"
#include "fboss/agent/platforms/common/minipack3bta/Minipack3BTAPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/minipack3bta/Minipack3BTABspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Minipack3BTALedManager ctor()
 *
 * Minipack3BTALedManager constructor will create the LedManager object for
 * Minipack3BTA platform
 */
Minipack3BTALedManager::Minipack3BTALedManager() : BspLedManager() {
  init<Minipack3BTABspPlatformMapping, Minipack3BTAPlatformMapping>();
  XLOG(INFO) << "Created Minipack3BTA BSP LED Manager";
}

} // namespace facebook::fboss
