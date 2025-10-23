// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Ladakh800bcLedManager.h"
#include "fboss/agent/platforms/common/ladakh800bc/Ladakh800bcPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/ladakh800bc/Ladakh800bcBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Ladakh800bcLedManager ctor()
 *
 * Ladakh800bcLedManager constructor will create the LedManager object for
 * Ladakh800bc platform
 */
Ladakh800bcLedManager::Ladakh800bcLedManager() : BspLedManager() {
  init<Ladakh800bcBspPlatformMapping, Ladakh800bcPlatformMapping>();
  XLOG(INFO) << "Created Ladakh800bc BSP LED Manager";
}

} // namespace facebook::fboss
