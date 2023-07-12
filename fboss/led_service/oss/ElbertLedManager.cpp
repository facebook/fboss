// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/ElbertLedManager.h"

namespace facebook::fboss {

/*
 * ElbertLedManager ctor()
 *
 * ElbertLedManager constructor will just call based LedManager ctor() as of
 * now.
 */
ElbertLedManager::ElbertLedManager() : LedManager() {}

/*
 * calculateLedColor
 *
 * This function will return the LED color for a given port.
 */
led::LedColor ElbertLedManager::calculateLedColor(
    uint32_t portId,
    cfg::PortProfileID /* portProfile */) const {
  return led::LedColor::UNKNOWN;
}

/*
 * setLedColor
 *
 * Set the LED color in HW for the LED on a given port. This function should
 * not depend on FSDB provided values from portDisplayMap_
 */
void ElbertLedManager::setLedColor(
    uint32_t portId,
    cfg::PortProfileID /* portProfile */,
    led::LedColor ledColor) {
  return;
}

} // namespace facebook::fboss
