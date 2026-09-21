// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Janga800bicLedManager.h"
#include "fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/janga800bic/Janga800bicBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Janga800bicLedManager ctor()
 *
 * Janga800bicLedManager constructor will create the LedManager object for
 * Janga800bic platform
 */
Janga800bicLedManager::Janga800bicLedManager() : BspLedManager() {
  init<Janga800bicBspPlatformMapping, Janga800bicPlatformMapping>();
  XLOG(INFO) << "Created Janga800bic BSP LED Manager";
}

} // namespace facebook::fboss
