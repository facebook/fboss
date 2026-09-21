/*
 *  Copyright (c) 2026 Nexthop Systems Inc.
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "fboss/led_service/M4062nhpLedManager.h"
#include "fboss/lib/bsp/m4062nhp/M4062nhpBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * M4062nhpLedManager ctor()
 *
 * M4062nhpLedManager constructor will create the LedManager object for
 * m4062nhp platform
 */
M4062nhpLedManager::M4062nhpLedManager() : BspLedManager() {
  init<M4062nhpBspPlatformMapping>(PlatformType::PLATFORM_M4062NHP);
  XLOG(INFO) << "Created M4062nhp BSP LED Manager";
}

} // namespace facebook::fboss
