// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Leh800bclsLedManager.h"
#include "fboss/agent/platforms/common/leh800bcls/Leh800bclsPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/leh800bcls/Leh800bclsBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Leh800bclsLedManager ctor()
 *
 * Leh800bclsLedManager constructor will create the LedManager object for
 * Leh800bcls platform
 */
Leh800bclsLedManager::Leh800bclsLedManager() : BspLedManager() {
  init<Leh800bclsBspPlatformMapping, Leh800bclsPlatformMapping>();
  XLOG(INFO) << "Created Leh800bcls BSP LED Manager";
}

} // namespace facebook::fboss
