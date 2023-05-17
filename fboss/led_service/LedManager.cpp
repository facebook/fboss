// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/LedManager.h"
#include "fboss/agent/EnumUtils.h"

namespace facebook::fboss {

/*
 * LedManager ctor()
 *
 * LedManager constructor will create PubSub manager to subscribe to the
 * SwitchState update on FSDB. The switch state map
 * subscribedAgentSwitchStatePortData_ is updated by callback from
 * FsdbLedSubscriber and the updates used here
 */
LedManager::LedManager() {
  eventBase_ = std::make_unique<folly::EventBase>();
  auto* evb = eventBase_.get();
  ledManagerThread_ =
      std::make_unique<std::thread>([=] { evb->loopForever(); });

  XLOG(INFO) << "Created base LED Manager and subscribed to FSDB";
}

LedManager::~LedManager() {
  if (eventBase_) {
    eventBase_->terminateLoopSoon();
  }
  if (ledManagerThread_) {
    ledManagerThread_->join();
  }
}

/*
 * getLedIdFromSwPort
 *
 * The function returns list of LED Id for a given SW Port and Profile. One SW
 * port may have more than one LED.
 */
std::vector<int> LedManager::getLedIdFromSwPort(
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

  std::unordered_set<int> ledIdSet;
  for (auto tcvrLane : tcvrHostLanes) {
    auto ledId = bspSystemContainer_->getBspPlatformMapping()->getLedId(
        tcvrId, tcvrLane);
    ledIdSet.insert(ledId);
  }
  return std::vector<int>(ledIdSet.begin(), ledIdSet.end());
}

/*
 * getCommonLedSwPorts
 *
 * Multiple SW Ports can display the status through same LED in some platforms.
 * This function helps in that scenario. If more than one SW port is
 * represented by one LED then this function returns the list of all common SW
 * port which is sharing the LED with the given port. Ths function does:
 * 1. Find transceiver Id for this SW Port
 * 2. Get all SW port list supported by this transceiver (irrespective of the
 *    port profile)
 * 3. Check if any of the other SW port in the same transceiver has the same
 *    Led ids as current port (while checking this, use each port's respective
 *    port profile because these ports can have different profiles)
 * 4. Make a list of all SW ports whose LED Id matches (partially or fully)
 *    with the current port Led ids
 */
std::vector<uint32_t> LedManager::getCommonLedSwPorts(
    uint32_t portId,
    cfg::PortProfileID portProfile) const {
  auto tcvrId = platformMapping_->getTransceiverIdFromSwPort(PortID(portId));
  auto allSwPortsForTcvr =
      platformMapping_->getSwPortListFromTransceiverId(tcvrId);

  auto anyLedMatch = [](const std::vector<int>& ledList1,
                        const std::vector<int>& ledList2) -> bool {
    for (auto led1 : ledList1) {
      if (std::find(ledList2.begin(), ledList2.end(), led1) != ledList2.end()) {
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
        portDisplayList_.find(swPort) == portDisplayList_.end()) {
      continue;
    }

    auto thisPortProfile = portDisplayList_.at(swPort).portProfileId;
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
 * act on LedManager struct portDisplayList_ to find the color. This function
 * expects the port oprational values (ie: portDisplayList_.operationalStateUp)
 * is already updated with latest. This function takes care of scenario where
 * the LED is shared by multiple SW ports
 */
led::LedColor LedManager::calculateLedColor(
    uint32_t portId,
    cfg::PortProfileID portProfile) {
  if (portDisplayList_.find(portId) == portDisplayList_.end()) {
    XLOG(ERR) << folly::sformat(
        "Port {:d} LED color undetermined as the port operational info is not available",
        portId);
    return led::LedColor::UNKNOWN;
  }

  // Get all the SW ports which share the LED with the current port. Then find
  // port up, reachability info for all common ports and then decide LED color

  auto commonSwPorts = getCommonLedSwPorts(portId, portProfile);
  if (commonSwPorts.empty()) {
    XLOG(ERR) << folly::sformat("Port {:d} LED Id unavailable", portId);
    return led::LedColor::UNKNOWN;
  }

  bool anyPortUp{false}, allPortsUp{true};
  bool anyPortReachable{false}, allPortsReachable{true};

  for (auto swPort : commonSwPorts) {
    if (portDisplayList_.find(swPort) == portDisplayList_.end()) {
      continue;
    }

    auto thisPortUp = portDisplayList_[swPort].operationStateUp;
    anyPortUp = anyPortUp || thisPortUp;
    allPortsUp = allPortsUp && thisPortUp;

    auto thisPortReachable = portDisplayList_[swPort].neighborReachable;
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
led::LedColor LedManager::getLedColorFromPortStatus(
    bool anyPortUp,
    bool allPortsUp,
    bool allPortsReachable) {
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

} // namespace facebook::fboss
