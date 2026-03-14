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
    auto itr = portDisplayMap_.find(swPort);
    if (PortID(portId) == swPort || itr == portDisplayMap_.end()) {
      continue;
    }

    auto thisPortProfile = itr->second.portProfileId;
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
  const auto portNamePlatformMapping =
      platformMapping_->getPortNameByPortId(PortID(portId)).value_or("UNKNOWN");

  // Get all the SW ports which share the LED with the current port. Then find
  // port up, reachability info for all common ports and then decide LED color

  auto commonSwPorts = getCommonLedSwPorts(portId, portProfile);
  if (commonSwPorts.empty()) {
    return utility::constructLedState(
        led::LedColor::UNKNOWN, led::Blink::UNKNOWN);
  }

  PortNeighborState pNState;
  for (auto swPort : commonSwPorts) {
    auto itr2 = portDisplayMap_.find(swPort);
    if (itr2 == portDisplayMap_.end()) {
      continue;
    }
    const auto& portDisplayInfo = itr2->second;
    pNState.totalPorts++;
    pNState.portsUpAndCorrectReachability =
        portDisplayInfo.operationStateUp && !portDisplayInfo.cablingError
        ? pNState.portsUpAndCorrectReachability + 1
        : pNState.portsUpAndCorrectReachability;
    pNState.anyPortUp |= portDisplayInfo.operationStateUp;

    pNState.anyForcedOn |= portDisplayInfo.forcedOn;
    pNState.anyForcedOff |= portDisplayInfo.forcedOff;
    pNState.anyUndrainedPort |= !portDisplayInfo.drained;

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
          pNState.portsWithAllLanesRxLos++;
        }
      }
    }
  }

  // Sanity check warning
  if (pNState.anyForcedOn && pNState.anyForcedOff) {
    XLOG(WARN) << fmt::format(
        "Port {:s} LED is Forced inconsistently On and Off. Forcing it on",
        portName);
    pNState.anyForcedOn = true;
    pNState.anyForcedOff = false;
  }

  // Foced LED value overrides the status
  if (pNState.anyForcedOn) {
    XLOG(DBG2) << fmt::format("Port {:d} Forced On", portId);
    return utility::constructLedState(led::LedColor::BLUE, led::Blink::OFF);
  } else if (pNState.anyForcedOff) {
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

  if (pNState.anyUndrainedPort) {
    currBlink = led::Blink::OFF;
    if (pNState.portsUpAndCorrectReachability == pNState.totalPorts) {
      currPortColor = led::LedColor::BLUE;
    } else if (pNState.anyPortUp) {
      currPortColor = led::LedColor::YELLOW;
    } else {
      currPortColor = led::LedColor::OFF;
    }
  } else {
    if (pNState.portsUpAndCorrectReachability == pNState.totalPorts) {
      currPortColor = led::LedColor::BLUE;
      currBlink = led::Blink::FAST;
    } else if (pNState.anyPortUp) {
      currPortColor = led::LedColor::YELLOW;
      currBlink = led::Blink::FAST;
    } else {
      // No port is up, now check if there is any port has some light incoming
      // vs no light
      if (pNState.portsWithAllLanesRxLos < pNState.totalPorts) {
        currPortColor = led::LedColor::BLUE;
        currBlink = led::Blink::SLOW;
      } else {
        currPortColor = led::LedColor::YELLOW;
        currBlink = led::Blink::SLOW;
      }
    }
  }

  auto newLedState = utility::constructLedState(currPortColor, currBlink);
  if (newLedState != itr->second.currentLedState ||
      pNState != itr->second.portNeighborState) {
    itr->second.portNeighborState = pNState;
    XLOG(ERR) << fmt::format(
        "Port {:s} {:s} ledChanged={:s} totalPorts={:d} portsUpAndCorrectReachability={:d} portsWithAllLanesRxLos={:d} anyUndrainedPort={:s} operState={:s} anyPortUp={:s} ledColor={:s} ledBlink={:s}",
        portName,
        portNamePlatformMapping,
        newLedState != itr->second.currentLedState ? "True" : "False",
        pNState.totalPorts,
        pNState.portsUpAndCorrectReachability,
        pNState.portsWithAllLanesRxLos,
        pNState.anyUndrainedPort ? "True" : "False",
        pNState.anyPortUp ? "Up" : "Down",
        itr->second.operationStateUp ? "Up" : "Down",
        apache::thrift::util::enumNameSafe(currPortColor),
        apache::thrift::util::enumNameSafe(currBlink));
  }
  return newLedState;
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

  // A special scenario where a single port controls more than 1 led and
  // the port is down and drained (i.e. Slow blink).
  // In this case, we will set the LEDs to match the LOS status per led to the
  // associated lanes to clarify which fibers are getting a signal v.s
  // ones not getting a signal.
  if (ledState.blink() == led::Blink::SLOW && ledIds.size() > 1) {
    // In this scenario, we want individual LEDs to represent the
    // LOS lanes in the port.
    setLedBasedOnLOS(portId, ledIds, ledControllers, ledState);
  } else {
    for (auto& ledController : ledControllers) {
      if (ledIds.count(ledController.first)) {
        ledController.second.first->setLedState(ledState);
      }
    }
  }
}

void BspLedManager::setLedBasedOnLOS(
    const uint32_t portId,
    const std::set<int>& ledIds,
    const std::map<uint32_t, std::pair<LedIO*, std::set<int>>>& controllers,
    led::LedState ledState) {
  auto itr = portLosMap_.find(portId);
  if (itr == portLosMap_.end()) {
    XLOG(ERR) << "No entry in LOS map for the port " << portId;
    return;
  }
  const auto& rxLos = itr->second.rxLos;
  if (!rxLos.has_value()) {
    XLOG(ERR) << "No Los value is set for the port " << portId;
    return;
  }
  const auto& losMap = rxLos.value();

  // For Each of the led controllers, check if all the lanes
  // in the led controller are seeing LOS, if so set individual
  // LED to Yellow.
  for (const auto& ledController : controllers) {
    if (ledIds.count(ledController.first)) {
      bool allLanesLos = true;
      for (const auto& lane : ledController.second.second) {
        auto itr2 = losMap.find(lane);
        if (itr2 == losMap.end()) {
          XLOG(ERR) << "No entry in LOS map for lane " << lane << " in port "
                    << portId;
        } else if (itr2->second == false) {
          allLanesLos = false;
          break;
        }
      }
      if (allLanesLos) {
        ledState.ledColor() = led::LedColor::YELLOW;
      } else {
        ledState.ledColor() = led::LedColor::BLUE;
      }
      XLOG(INFO) << "Los Control: Setting LED to "
                 << apache::thrift::util::enumNameSafe(
                        ledState.ledColor().value())
                 << " for port " << portId << " led " << ledController.first;
      ledController.second.first->setLedState(ledState);
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
      ledStates.insert(ledController.second.first->getLedState());
    }
  }
  return ledStates;
}

} // namespace facebook::fboss
