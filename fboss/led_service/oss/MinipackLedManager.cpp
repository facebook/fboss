// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/MinipackLedManager.h"

namespace facebook::fboss {

/*
 * MinipackLedManager ctor()
 *
 * MinipackLedManager constructor will just call based LedManager ctor() as of
 * now.
 */
MinipackLedManager::MinipackLedManager() : LedManager() {}

/*
 * calculateLedColor
 *
 * This function will return the LED color for a given port. This function will
 * act on LedManager struct portDisplayMap_ to find the color. This function
 * expects the port oprational values (ie: portDisplayMap_.operationalStateUp)
 * is already updated with latest.
 */
led::LedColor MinipackLedManager::calculateLedColor(
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
void MinipackLedManager::setLedColor(
    uint32_t portId,
    cfg::PortProfileID portProfile,
    led::LedColor ledColor) {
  return;
}

} // namespace facebook::fboss
