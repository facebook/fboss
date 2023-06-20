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
 * calculateLedColor
 *
 * This function will return the LED color for a given port.
 */
led::LedColor LassenLedManager::calculateLedColor(
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
void LassenLedManager::setLedColor(
    uint32_t portId,
    cfg::PortProfileID /* portProfile */,
    led::LedColor ledColor) {
  return;
}

} // namespace facebook::fboss
