// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/MinipackBaseLedManager.h"
#include "fboss/agent/EnumUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonPortUtils.h"
#include "fboss/lib/fpga/MinipackLed.h"

namespace facebook::fboss {

/*
 * MinipackBaseLedManager ctor()
 *
 * MinipackBaseLedManager constructor will just call based LedManager ctor() as
 * of now.
 */
MinipackBaseLedManager::MinipackBaseLedManager() : LedManager() {}

/*
 * calculateLedColor
 *
 * This function will return the LED color for a given port. This function will
 * act on LedManager struct portDisplayMap_ to find the color. This function
 * expects the port oprational values (ie: portDisplayMap_.operationalStateUp)
 * is already updated with latest. This function will take care of port
 * operational state, LLDP cabling error, user forced LED color also
 *
 * TODO(rajank):
 * 1. Add forced LED case here
 */
led::LedColor MinipackBaseLedManager::calculateLedColor(
    uint32_t portId,
    cfg::PortProfileID /* portProfile */) const {
  if (portDisplayMap_.find(portId) == portDisplayMap_.end()) {
    XLOG(ERR) << folly::sformat(
        "Port {:d} LED color undetermined as the port operational info is not available",
        portId);
    return led::LedColor::UNKNOWN;
  }

  auto portName = portDisplayMap_.at(portId).portName;
  auto portUp = portDisplayMap_.at(portId).operationStateUp;
  auto cablingError = portDisplayMap_.at(portId).cablingError;
  auto ledColor = led::LedColor::UNKNOWN;
  // Cabling error overrides the port status
  if (cablingError) {
    ledColor = led::LedColor::YELLOW;
  } else {
    ledColor = portUp ? led::LedColor::BLUE : led::LedColor::OFF;
  }

  XLOG(DBG2) << fmt::format(
      "Port {:s}, portUp={:s}, cablingError={:s}",
      portName,
      (portUp ? "True" : "False"),
      (cablingError ? "True" : "False"));

  return ledColor;
}

} // namespace facebook::fboss
