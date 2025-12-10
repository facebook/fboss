// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Wedge800BACTLedManager.h"
#include "fboss/agent/platforms/common/wedge800bact/Wedge800BACTPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/wedge800bact/Wedge800BACTBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Wedge800BACTLedManager ctor()
 *
 * Wedge800BACTLedManager constructor will create the LedManager object for
 * Wedge800BACT platform
 */
Wedge800BACTLedManager::Wedge800BACTLedManager() : BspLedManager() {
  init<Wedge800BACTBspPlatformMapping, Wedge800BACTPlatformMapping>();
  XLOG(INFO) << "Created Wedge800BACT BSP LED Manager";
}

} // namespace facebook::fboss
