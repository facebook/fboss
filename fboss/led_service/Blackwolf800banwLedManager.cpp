// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Blackwolf800banwLedManager.h"
#include "fboss/agent/platforms/common/blackwolf800banw/Blackwolf800banwPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/blackwolf800banw/Blackwolf800banwBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Blackwolf800banwLedManager ctor()
 *
 * Blackwolf800banwLedManager constructor will create the LedManager object for
 * blackwolf800banw platform
 */
Blackwolf800banwLedManager::Blackwolf800banwLedManager() : BspLedManager() {
  init<Blackwolf800banwBspPlatformMapping, Blackwolf800banwPlatformMapping>();
  XLOG(INFO) << "Created Blackwolf800banw BSP LED Manager";
}

} // namespace facebook::fboss
