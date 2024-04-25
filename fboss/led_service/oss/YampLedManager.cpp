// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/YampLedManager.h"

namespace facebook::fboss {

/*
 * YampLedManager ctor()
 *
 * YampLedManager constructor will just call based LedManager ctor() as of
 * now.
 */
YampLedManager::YampLedManager() : LedManager() {}

/*
 * calculateLedState
 *
 * This function will return the LED color for a given port.
 */
led::LedState YampLedManager::calculateLedState(
    uint32_t portId,
    cfg::PortProfileID /* portProfile */) const {
  return utility::constructLedState(
      led::LedColor::UNKNOWN, led::Blink::UNKNOWN);
}

/*
 * setLedColor
 *
 * Set the LED color in HW for the LED on a given port. This function should
 * not depend on FSDB provided values from portDisplayMap_
 */
void YampLedManager::setLedColor(
    uint32_t portId,
    cfg::PortProfileID /* portProfile */,
    led::LedColor ledColor) {
  return;
}

} // namespace facebook::fboss
