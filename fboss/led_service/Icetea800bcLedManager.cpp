// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Icetea800bcLedManager.h"
#include "fboss/agent/platforms/common/icetea800bc/Icetea800bcPlatformMapping.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include "fboss/lib/bsp/icetea800bc/Icetea800bcBspPlatformMapping.h"

namespace facebook::fboss {

/*
 * Icetea800bcLedManager ctor()
 *
 * Icetea800bcLedManager constructor will create the LedManager object for
 * Icetea800bc platform
 */
Icetea800bcLedManager::Icetea800bcLedManager() : BspLedManager() {
  init<Icetea800bcBspPlatformMapping, Icetea800bcPlatformMapping>();
  XLOG(INFO) << "Created Icetea800bc BSP LED Manager";
}

} // namespace facebook::fboss
