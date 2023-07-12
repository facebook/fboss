// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/FujiLedManager.h"

namespace facebook::fboss {

/*
 * FujiLedManager ctor()
 *
 * FujiLedManager constructor will just call based LedManager ctor() as of
 * now.
 */
FujiLedManager::FujiLedManager() : MinipackBaseLedManager() {}

/*
 * setLedColor
 *
 * Set the LED color in HW for the LED on a given port. This function should
 * not depend on FSDB provided values from portDisplayMap_
 */
void FujiLedManager::setLedColor(
    uint32_t portId,
    cfg::PortProfileID portProfile,
    led::LedColor ledColor) {
  return;
}

} // namespace facebook::fboss
