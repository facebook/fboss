// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Tahan800bcLedManager.h"
#include "fboss/agent/platforms/common/tahan800bc/Tahan800bcPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/tahan800bc/Tahan800bcBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Tahan800bcLedManager ctor()
 *
 * Tahan800bcLedManager constructor will create the LedManager object for
 * Tahan800bc platform
 */
Tahan800bcLedManager::Tahan800bcLedManager() : BspLedManager() {
  init<Tahan800bcBspPlatformMapping, Tahan800bcPlatformMapping>();
  XLOG(INFO) << "Created Tahan800bc BSP LED Manager";
}

} // namespace facebook::fboss
