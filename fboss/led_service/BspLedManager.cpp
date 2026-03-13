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
  auto itr = portDisplayMap_.find(portId);
  if (itr == portDisplayMap_.end()) {
    XLOG(ERR) << folly::sformat(
        "Port {:d} LED color undetermined as the port operational info is not available",
        portId);
    return utility::constructLedState(
        led::LedColor::UNKNOWN, led::Blink::UNKNOWN);
  }
  const auto& portName = itr->second.portName;

  // Get all the SW ports which share the LED with the current port. Then find
  // port up, reachability info for all common ports and then decide LED color

  auto commonSwPorts = getCommonLedSwPorts(portId, portProfile);
  if (commonSwPorts.empty()) {
    return utility::constructLedState(
        led::LedColor::UNKNOWN, led::Blink::UNKNOWN);
  }

  uint32_t totalPorts{0}, portsUpAndCorrectReachability{0},
      portsWithAllLanesRxLos{0};
  bool anyForcedOn{false}, anyForcedOff{false};
  bool anyUndrainedPort{false};
  bool anyPortUp{false};

  // todo: need to remove activeState from portDisplayMap

  for (auto swPort : commonSwPorts) {
    auto itr2 = portDisplayMap_.find(swPort);
    if (itr2 == portDisplayMap_.end()) {
      continue;
    }
    const auto& portDisplayInfo = itr2->second;
    totalPorts++;
    portsUpAndCorrectReachability =
        portDisplayInfo.operationStateUp && !portDisplayInfo.cablingError
        ? portsUpAndCorrectReachability + 1
        : portsUpAndCorrectReachability;
    anyPortUp |= portDisplayInfo.operationStateUp;

    anyForcedOn |= portDisplayInfo.forcedOn;
    anyForcedOff |= portDisplayInfo.forcedOff;
    anyUndrainedPort |= !portDisplayInfo.drained;

    auto itrLos = portLosMap_.find(swPort);
    if (itrLos != portLosMap_.end()) {
      auto& losInfo = itrLos->second;
      if (losInfo.rxLos.has_value()) {
        // in case transceiver supports rxLos, we will use it to determine LED
        bool allLanesRxLos = true;
        for (auto& [lane, rxLos] : losInfo.rxLos.value()) {
          if (!rxLos) {
            allLanesRxLos = false;
            break;
          }
        }
        if (allLanesRxLos) {
          portsWithAllLanesRxLos++;
        }
      }
    }
  }

  // Sanity check warning
  if (anyForcedOn && anyForcedOff) {
    XLOG(WARN) << fmt::format(
        "Port {:s} LED is Forced inconsistently On and Off. Forcing it on",
        portName);
    anyForcedOn = true;
    anyForcedOff = false;
  }

  // Foced LED value overrides the status
  if (anyForcedOn) {
    XLOG(DBG2) << fmt::format("Port {:d} Forced On", portId);
    return utility::constructLedState(led::LedColor::BLUE, led::Blink::OFF);
  } else if (anyForcedOff) {
    XLOG(DBG2) << fmt::format("Port {:d} Forced Off", portId);
    return utility::constructLedState(led::LedColor::OFF, led::Blink::OFF);
  }

  /*
   * BSP LED color scheme:
   * Undrained Port
   *  # of ports UP and have correct reachability == all    -> BLUE
   *  0 < # of ports UP and have correct reachability < all -> AMBER
   *  # of ports UP and have correct reachability == 0      -> OFF
   *
   * Drained Port
   *  # of ports UP and have correct reachability == all
   *                  -> FAST FLASHING BLUE
   *  0 < # of ports UP and have correct reachability < all
   *                  -> FAST FLASHING AMBER
   *  # of ports UP and have correct reachability == 0 and some ports LOS
   *                  -> SLOW FLASHING BLUE
   *  # of ports UP and have correct reachability == 0 and all ports LOS
   *                  -> SLOW FLASHING AMBER
   */
  led::LedColor currPortColor{led::LedColor::OFF};
  led::Blink currBlink{led::Blink::OFF};

  if (anyUndrainedPort) {
    currBlink = led::Blink::OFF;
    if (portsUpAndCorrectReachability == totalPorts) {
      currPortColor = led::LedColor::BLUE;
    } else if (anyPortUp) {
      currPortColor = led::LedColor::YELLOW;
    } else {
      currPortColor = led::LedColor::OFF;
    }
  } else {
    if (portsUpAndCorrectReachability == totalPorts) {
      currPortColor = led::LedColor::BLUE;
      currBlink = led::Blink::FAST;
    } else if (anyPortUp) {
      currPortColor = led::LedColor::YELLOW;
      currBlink = led::Blink::FAST;
    } else {
      // portsUpAndCorrectReachability == 0
      if (portsWithAllLanesRxLos < totalPorts) {
        currPortColor = led::LedColor::BLUE;
        currBlink = led::Blink::SLOW;
      } else {
        currPortColor = led::LedColor::YELLOW;
        currBlink = led::Blink::SLOW;
      }
    }
  }

  XLOG(DBG3) << fmt::format(
      "Port {:s}, totalPorts={:d} portsUpAndCorrectReachability={:d} portsWithAllLanesRxLos={:d} anyUndrainedPort={:s} ledColor={:s} ledBlink={:s}",
      portName,
      totalPorts,
      portsUpAndCorrectReachability,
      portsWithAllLanesRxLos,
      anyUndrainedPort ? "True" : "False",
      apache::thrift::util::enumNameSafe(currPortColor),
      apache::thrift::util::enumNameSafe(currBlink));
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
  auto ledControllers = bspSystemContainer_->getLedController(tcvrId + 1);

  for (auto& ledController : ledControllers) {
    if (ledIds.count(ledController.first)) {
      ledController.second->setLedState(ledState);
    }
  }
}

/*
 * getLedStateFromHW
 *
 * Get the LED state from HW for the LED(s) on a given port. A port may have
 * multiple LEDs so this returns a set of LedState. This function reads the
 * actual sysfs files rather than cached state.
 */
std::set<led::LedState> BspLedManager::getLedStateFromHW(
    uint32_t portId) const {
  if (portDisplayMap_.find(portId) == portDisplayMap_.end()) {
    XLOG(ERR) << "Port " << portId
              << " not found in portDisplayMap for getLedStateFromHW";
    return {};
  }

  auto portProfile = portDisplayMap_.at(portId).portProfileId;
  auto ledIds = getLedIdFromSwPort(portId, portProfile);
  if (ledIds.empty()) {
    XLOG(ERR) << "No Led Id found for the port " << portId;
    return {};
  }

  auto tcvrId = platformMapping_->getTransceiverIdFromSwPort(PortID(portId));

  std::set<led::LedState> ledStates;
  for (auto& ledController :
       bspSystemContainer_->getLedController(tcvrId + 1)) {
    if (ledIds.count(ledController.first)) {
      ledStates.insert(ledController.second->getLedState());
    }
  }
  return ledStates;
}

} // namespace facebook::fboss
