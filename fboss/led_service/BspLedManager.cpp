// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/BspLedManager.h"
#include "fboss/agent/EnumUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonPortUtils.h"
#include "fboss/lib/bsp/BspGenericSystemContainer.h"

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
        tcvrId, tcvrLane);
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
 * calculateLedColor
 *
 * This function will return the LED color for a given port. This function will
 * act on LedManager struct portDisplayMap_ to find the color. This function
 * expects the port oprational values (ie: portDisplayMap_.operationalStateUp)
 * is already updated with latest. This function takes care of scenario where
 * the LED is shared by multiple SW ports
 */
led::LedColor BspLedManager::calculateLedColor(
    uint32_t portId,
    cfg::PortProfileID portProfile) const {
  if (portDisplayMap_.find(portId) == portDisplayMap_.end()) {
    XLOG(ERR) << folly::sformat(
        "Port {:d} LED color undetermined as the port operational info is not available",
        portId);
    return led::LedColor::UNKNOWN;
  }

  // Get all the SW ports which share the LED with the current port. Then find
  // port up, reachability info for all common ports and then decide LED color

  auto commonSwPorts = getCommonLedSwPorts(portId, portProfile);
  if (commonSwPorts.empty()) {
    return led::LedColor::UNKNOWN;
  }

  bool anyPortUp{false}, allPortsUp{true};
  bool anyPortReachable{false}, allPortsReachable{true};

  for (auto swPort : commonSwPorts) {
    if (portDisplayMap_.find(swPort) == portDisplayMap_.end()) {
      continue;
    }

    auto thisPortUp = portDisplayMap_.at(swPort).operationStateUp;
    anyPortUp |= thisPortUp;
    allPortsUp &= thisPortUp;

    auto thisPortReachable = portDisplayMap_.at(swPort).neighborReachable;
    anyPortReachable = anyPortReachable || thisPortReachable;
    allPortsReachable = allPortsReachable && thisPortReachable;
  }

  XLOG(DBG2) << fmt::format(
      "Port {:d}, anyPortUp={:s} allPortsUp={:s} anyPortReachable={:s} allPortsReachable={:s}",
      portId,
      (anyPortUp ? "True" : "False"),
      (allPortsUp ? "True" : "False"),
      (anyPortReachable ? "True" : "False"),
      (allPortsReachable ? "True" : "False"));

  return getLedColorFromPortStatus(anyPortUp, allPortsUp, allPortsReachable);
}

/*
 * getLedColorFromPortStatus
 *
 * Helper function to determine the LED color based on how many ports (attached
 * to the same LED) are Up and if all are reachable from neighbors.
 * Default LED scheme:
 *     All ports Up and neighbor reachable       -> Blue
 *     Some ports down or unreachable            -> Yellow
 *     All ports Down                            -> Off
 */
led::LedColor BspLedManager::getLedColorFromPortStatus(
    bool anyPortUp,
    bool allPortsUp,
    bool allPortsReachable) const {
  led::LedColor currPortColor{led::LedColor::UNKNOWN};

  if (!anyPortUp) {
    currPortColor = led::LedColor::OFF;
  } else if (allPortsUp && allPortsReachable) {
    currPortColor = led::LedColor::BLUE;
  } else {
    currPortColor = led::LedColor::YELLOW;
  }
  return currPortColor;
}

/*
 * setLedColor
 *
 * Set the LED color in HW for the LED(s) on a given port. This function will
 * find all the LED for the SW port and set their color. This function should
 * not depend on FSDB provided values from portDisplayMap_
 */
void BspLedManager::setLedColor(
    uint32_t portId,
    cfg::PortProfileID portProfile,
    led::LedColor ledColor) {
  auto ledIds = getLedIdFromSwPort(portId, portProfile);
  if (ledIds.empty()) {
    XLOG(ERR) << "No Led Id found or the port " << portId;
    return;
  }

  auto tcvrId = platformMapping_->getTransceiverIdFromSwPort(PortID(portId));

  for (auto& ledController : bspSystemContainer_->getLedController(tcvrId)) {
    if (std::find(ledIds.begin(), ledIds.end(), ledController.first) !=
        ledIds.end()) {
      ledController.second->setColor(ledColor);
    }
  }
}

} // namespace facebook::fboss
