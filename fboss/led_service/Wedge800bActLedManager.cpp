// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Wedge800bActLedManager.h"
#include "fboss/agent/platforms/common/wedge800b_act/Wedge800bActPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/wedge800b_act/Wedge800bActBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Wedge800bActLedManager ctor()
 *
 * Wedge800bActLedManager constructor will create the LedManager object for
 * Wedge800bAct platform
 */
Wedge800bActLedManager::Wedge800bActLedManager() : BspLedManager() {
  init<Wedge800bActBspPlatformMapping, Wedge800bActPlatformMapping>();
  XLOG(INFO) << "Created Wedge800bAct BSP LED Manager";
}

} // namespace facebook::fboss
