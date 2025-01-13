// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/BspLedManager.h"
#include <optional>
#include "fboss/agent/EnumUtils.h"
#include "fboss/lib/CommonFileUtils.h"

namespace facebook::fboss {

/*
 * BspLedManager ctor()
 *
 * BspLedManager constructor will create the LedManager object for BSP based
 * platforms
 */
BspLedManager::BspLedManager() : LedManager() {}

/*
 * getLedIdFromSwPort
 *
 * The function returns list of LED Id for a given SW Port and Profile. One SW
 * port may have more than one LED.
 */
std::set<int> BspLedManager::getLedIdFromSwPort(
    uint32_t portId,
    cfg::PortProfileID portProfile) const {
  PlatformPortProfileConfigMatcher matcher(
      portProfile /* profileID */,
      PortID(portId) /* portID */,
      std::nullopt /* portConfigOverrideFactor */);

  auto tcvrId = platformMapping_->getTransceiverIdFromSwPort(PortID(portId));

  auto tcvrHostLanes = platformMapping_->getTransceiverHostLanes(matcher);
  if (tcvrHostLanes.empty()) {
    XLOG(ERR) << "Transceiver host lane empty for port " << portId;
    return {};
  }

  std::set<int> ledIdSet;
  for (auto tcvrLane : tcvrHostLanes) {
    auto ledId = bspSystemContainer_->getBspPlatformMapping()->getLedId(
        tcvrId + 1, tcvrLane + 1);
    ledIdSet.insert(ledId);
  }
  return ledIdSet;
}

/*
 * getCommonLedSwPorts
 *
 * Multiple SW Ports can display the status through same LED in some platforms.
 * This function helps in that scenario. This function returns the list of all
 * common SW port which is sharing the LED with the given port. Ths function
 * does:
 * 1. Find transceiver Id for this SW Port
 * 2. Get all SW port list supported by this transceiver (irrespective of the
 *    port profile)
 * 3. Check if any of the other SW port in the same transceiver has the same
 *    Led ids as current port (while checking this, use each port's respective
 *    port profile because these ports can have different profiles)
 * 4. Make a list of all SW ports whose LED Id matches (partially or fully)
 *    with the current port Led ids
 */
std::vector<uint32_t> BspLedManager::getCommonLedSwPorts(
    uint32_t portId,
    cfg::PortProfileID portProfile) const {
  auto tcvrId = platformMapping_->getTransceiverIdFromSwPort(PortID(portId));
  auto allSwPortsForTcvr =
      platformMapping_->getSwPortListFromTransceiverId(tcvrId);

  auto anyLedMatch = [](const std::set<int>& ledList1,
                        const std::set<int>& ledList2) -> bool {
    for (auto led1 : ledList1) {
      if (ledList2.find(led1) != ledList2.end()) {
        return true;
      }
    }
    return false;
  };

  auto currSwPortLedIds = getLedIdFromSwPort(portId, portProfile);
  if (currSwPortLedIds.empty()) {
    XLOG(ERR) << "No LED Id available for port " << portId;
    return {};
  }

  std::vector<uint32_t> commonSwPorts;
  commonSwPorts.push_back(portId);

  for (auto swPort : allSwPortsForTcvr) {
    if (PortID(portId) == swPort ||
        portDisplayMap_.find(swPort) == portDisplayMap_.end()) {
      continue;
    }

    auto thisPortProfile = portDisplayMap_.at(swPort).portProfileId;
    auto thisSwPortLedIds = getLedIdFromSwPort(swPort, thisPortProfile);
    if (anyLedMatch(currSwPortLedIds, thisSwPortLedIds)) {
      commonSwPorts.push_back(swPort);
    }
  }
  return commonSwPorts;
}

/*
 * calculateLedState
 *
 * This function will return the LED color for a given port. This function will
 * act on LedManager struct portDisplayMap_ to find the color. This function
 * expects the port oprational values (ie: portDisplayMap_.operationalStateUp)
 * is already updated with latest. This function takes care of scenario where
 * the LED is shared by multiple SW ports
 */
led::LedState BspLedManager::calculateLedState(
    uint32_t portId,
    cfg::PortProfileID portProfile) const {
  if (portDisplayMap_.find(portId) == portDisplayMap_.end()) {
    XLOG(ERR) << folly::sformat(
        "Port {:d} LED color undetermined as the port operational info is not available",
        portId);
    return utility::constructLedState(
        led::LedColor::UNKNOWN, led::Blink::UNKNOWN);
  }

  // Get all the SW ports which share the LED with the current port. Then find
  // port up, reachability info for all common ports and then decide LED color

  auto commonSwPorts = getCommonLedSwPorts(portId, portProfile);
  if (commonSwPorts.empty()) {
    return utility::constructLedState(
        led::LedColor::UNKNOWN, led::Blink::UNKNOWN);
  }

  bool anyPortUp{false}, allPortsUp{true};
  bool anyCablingError{false};
  bool anyForcedOn{false}, anyForcedOff{false};
  std::optional<bool> anyActivePort{std::nullopt};
  bool anyUndrainedPort{false};

  for (auto swPort : commonSwPorts) {
    if (portDisplayMap_.find(swPort) == portDisplayMap_.end()) {
      continue;
    }

    auto thisPortUp = portDisplayMap_.at(swPort).operationStateUp;
    anyPortUp |= thisPortUp;
    allPortsUp &= thisPortUp;

    anyCablingError |= portDisplayMap_.at(swPort).cablingError;
    anyForcedOn |= portDisplayMap_.at(swPort).forcedOn;
    anyForcedOff |= portDisplayMap_.at(swPort).forcedOff;
    anyUndrainedPort |= !portDisplayMap_.at(swPort).drained;

    if (portDisplayMap_.at(swPort).activeState.has_value()) {
      if (!anyActivePort.has_value()) {
        anyActivePort = *portDisplayMap_.at(swPort).activeState;
      }
      anyActivePort = *anyActivePort || *portDisplayMap_.at(swPort).activeState;
    }
  }

  // Sanity check warning
  if (anyForcedOn && anyForcedOff) {
    XLOG(WARN) << fmt::format(
        "Port {:d} LED is Forced inconsistently On and Off", portId);
  }

  // Foced LED value overrides the status
  if (anyForcedOn) {
    XLOG(DBG2) << fmt::format("Port {:d} Forced On", portId);
    return utility::constructLedState(led::LedColor::BLUE, led::Blink::OFF);
  } else if (anyForcedOff) {
    XLOG(DBG2) << fmt::format("Port {:d} Forced Off", portId);
    return utility::constructLedState(led::LedColor::OFF, led::Blink::OFF);
  }

  XLOG(DBG3) << fmt::format(
      "Port {:d}, anyPortUp={:s} allPortsUp={:s} anyCablingError = {:s} anyActivePort={:s} anyUndrainedPort={:s}",
      portId,
      (anyPortUp ? "True" : "False"),
      (allPortsUp ? "True" : "False"),
      (anyCablingError ? "True" : "False"),
      (anyActivePort.has_value() ? (*anyActivePort ? "True" : "False") : "N/A"),
      (anyUndrainedPort ? "True" : "False"));

  /*
   * BSP LED color scheme:
   * Undrained Port
   *  # of ports UP and have correct reachability == 4    -> BLUE
   *  0 < # of ports UP and have correct reachability < 4 -> AMBER
   *  # of ports UP and have correct reachability == 0    -> OFF
   *
   * Drained Port
   *  # of ports UP and have correct reachability == 4    -> SLOW FLASHING BLUE
   *  0 < # of ports UP and have correct reachability < 4 -> FAST FLASHING AMBER
   *  # of ports UP and have correct reachability == 0    -> SLOW FLASHING AMBER
   */

  led::LedColor currPortColor{led::LedColor::UNKNOWN};
  led::Blink currBlink{led::Blink::OFF};

  if ((!anyActivePort.has_value() || *anyActivePort) && anyUndrainedPort) {
    currBlink = led::Blink::OFF;
    if (allPortsUp && !anyCablingError) {
      currPortColor = led::LedColor::BLUE;
    } else if (anyPortUp) {
      currPortColor = led::LedColor::YELLOW;
    } else {
      currPortColor = led::LedColor::OFF;
    }
  } else {
    if (allPortsUp && !anyCablingError) {
      currPortColor = led::LedColor::BLUE;
      currBlink = led::Blink::SLOW;
    } else if (anyPortUp) {
      currPortColor = led::LedColor::YELLOW;
      currBlink = led::Blink::FAST;
    } else {
      currPortColor = led::LedColor::OFF;
      currBlink = led::Blink::SLOW;
    }
  }

  return utility::constructLedState(currPortColor, currBlink);
}

/*
 * setLedState
 *
 * Set the LED color in HW for the LED(s) on a given port. This function will
 * find all the LED for the SW port and set their color. This function should
 * not depend on FSDB provided values from portDisplayMap_
 */
void BspLedManager::setLedState(
    uint32_t portId,
    cfg::PortProfileID portProfile,
    led::LedState ledState) {
  auto ledIds = getLedIdFromSwPort(portId, portProfile);
  if (ledIds.empty()) {
    XLOG(ERR) << "No Led Id found or the port " << portId;
    return;
  }

  auto tcvrId = platformMapping_->getTransceiverIdFromSwPort(PortID(portId));

  for (auto& ledController :
       bspSystemContainer_->getLedController(tcvrId + 1)) {
    if (std::find(ledIds.begin(), ledIds.end(), ledController.first) !=
        ledIds.end()) {
      ledController.second->setLedState(ledState);
    }
  }
}

} // namespace facebook::fboss
