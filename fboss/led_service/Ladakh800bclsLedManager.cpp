// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Ladakh800bclsLedManager.h"
#include "fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/ladakh800bcls/Ladakh800bclsBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Ladakh800bclsLedManager ctor()
 *
 * Ladakh800bclsLedManager constructor will create the LedManager object for
 * Ladakh800bcls platform
 */
Ladakh800bclsLedManager::Ladakh800bclsLedManager() : BspLedManager() {
  init<Ladakh800bclsBspPlatformMapping, Ladakh800bclsPlatformMapping>();
  XLOG(INFO) << "Created Ladakh800bcls BSP LED Manager";
}

} // namespace facebook::fboss
