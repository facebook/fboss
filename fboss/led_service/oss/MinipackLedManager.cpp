// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/MinipackLedManager.h"

namespace facebook::fboss {

/*
 * MinipackLedManager ctor()
 *
 * MinipackLedManager constructor will just call based LedManager ctor() as of
 * now.
 */
MinipackLedManager::MinipackLedManager() : MinipackBaseLedManager() {}

/*
 * setLedColor
 *
 * Set the LED color in HW for the LED on a given port. This function should
 * not depend on FSDB provided values from portDisplayMap_
 */
void MinipackLedManager::setLedColor(
    uint32_t portId,
    cfg::PortProfileID /* portProfile */,
    led::LedColor ledColor) {
  return;
}

} // namespace facebook::fboss
