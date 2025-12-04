// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Wedge800CACTLedManager.h"
#include "fboss/agent/platforms/common/wedge800cact/Wedge800CACTPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/wedge800cact/Wedge800CACTBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Wedge800CACTLedManager ctor()
 *
 * Wedge800CACTLedManager constructor will create the LedManager object for
 * Wedge800CACT platform
 */
Wedge800CACTLedManager::Wedge800CACTLedManager() : BspLedManager() {
  init<Wedge800CACTBspPlatformMapping, Wedge800CACTPlatformMapping>();
  XLOG(INFO) << "Created Wedge800CACT BSP LED Manager";
}

} // namespace facebook::fboss
