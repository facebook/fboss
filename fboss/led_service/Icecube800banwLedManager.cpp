// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Icecube800banwLedManager.h"
#include "fboss/agent/platforms/common/icecube800banw/Icecube800banwPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/icecube800banw/Icecube800banwBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Icecube800banwLedManager ctor()
 *
 * Icecube800banwLedManager constructor will create the LedManager object for
 * icecube800banw platform
 */
Icecube800banwLedManager::Icecube800banwLedManager() : BspLedManager() {
  init<Icecube800banwBspPlatformMapping, Icecube800banwPlatformMapping>();
  XLOG(INFO) << "Created Icecube800banw BSP LED Manager";
}

} // namespace facebook::fboss
