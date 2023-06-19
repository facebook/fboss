// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/Wedge400LedManager.h"

namespace facebook::fboss {

/*
 * Wedge400LedManager ctor()
 *
 * Wedge400LedManager constructor will just call based LedManager ctor() as of
 * now.
 */
Wedge400LedManager::Wedge400LedManager() : LedManager() {}

/*
 * calculateLedColor
 *
 * This function will return the LED color for a given port.
 */
led::LedColor Wedge400LedManager::calculateLedColor(
    uint32_t portId,
    cfg::PortProfileID portProfile) const {
  return led::LedColor::UNKNOWN;
}

/*
 * setLedColor
 *
 * Set the LED color in HW for the LED on a given port. This function should
 * not depend on FSDB provided values from portDisplayMap_
 */
void Wedge400LedManager::setLedColor(
    uint32_t portId,
    cfg::PortProfileID portProfile,
    led::LedColor ledColor) {
  return;
}

} // namespace facebook::fboss
