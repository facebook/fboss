// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/DarwinLedManager.h"
#include "fboss/agent/platforms/common/darwin/DarwinPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/darwin/DarwinBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * DarwinLedManager ctor()
 *
 * DarwinLedManager constructor will create the LedManager object for
 * Darwin platform
 */
DarwinLedManager::DarwinLedManager() : BspLedManager() {
  init<DarwinBspPlatformMapping, DarwinPlatformMapping>();
  XLOG(INFO) << "Created Darwin BSP LED Manager";
}

} // namespace facebook::fboss
