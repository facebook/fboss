// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/LassenLedManager.h"
#include "fboss/agent/EnumUtils.h"
#include "fboss/agent/platforms/common/lassen/LassenPlatformMapping.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonPortUtils.h"

namespace facebook::fboss {

/*
 * LassenLedManager ctor()
 *
 * LassenLedManager constructor will just call based LedManager ctor() as of
 * now.
 */
LassenLedManager::LassenLedManager() : LedManager() {}

/*
 * calculateLedState
 *
 * This function will return the LED color for a given port.
 */
led::LedState LassenLedManager::calculateLedState(
    uint32_t portId,
    cfg::PortProfileID /* portProfile */) const {
  return utility::constructLedState(
      led::LedColor::UNKNOWN, led::Blink::UNKNOWN);
}

/*
 * setLedState
 *
 * Set the LED color in HW for the LED on a given port. This function should
 * not depend on FSDB provided values from portDisplayMap_
 */
void LassenLedManager::setLedState(
    uint32_t portId,
    cfg::PortProfileID /* portProfile */,
    led::LedState ledState) {
  return;
}

} // namespace facebook::fboss
