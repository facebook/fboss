// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Wedge400BaseLedManager.h"

namespace facebook::fboss {

/*
 * Wedge400BaseLedManager ctor()
 *
 * Wedge400BaseLedManager constructor will just call based LedManager ctor() as
 * of now.
 */
Wedge400BaseLedManager::Wedge400BaseLedManager() : LedManager() {}

/*
 * calculateLedState
 *
 * This function will return the LED color for a given port.
 */
led::LedState Wedge400BaseLedManager::calculateLedState(
    uint32_t portId,
    cfg::PortProfileID portProfile) const {
  return utility::constructLedState(
      led::LedColor::UNKNOWN, led::Blink::UNKNOWN);
}

/*
 * setLedState
 *
 * Set the LED color in HW for the LED on a given port. This function should
 * not depend on FSDB provided values from portDisplayMap_
 */
void Wedge400BaseLedManager::setLedState(
    uint32_t portId,
    cfg::PortProfileID portProfile,
    led::LedState ledState) {
  return;
}

} // namespace facebook::fboss
