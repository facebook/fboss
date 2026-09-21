// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Meru800biaLedManager.h"
#include "fboss/agent/platforms/common/meru800bia/Meru800biaPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/meru800bia/Meru800biaBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Meru800biaLedManager ctor()
 *
 * Meru800biaLedManager constructor will create the LedManager object for
 * Meru800bia platform
 */
Meru800biaLedManager::Meru800biaLedManager() : BspLedManager() {
  init<Meru800biaBspPlatformMapping, Meru800biaPlatformMapping>();
  XLOG(INFO) << "Created Meru800bia BSP LED Manager";
}

} // namespace facebook::fboss
