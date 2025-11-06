// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/LedManager.h"
#include "fboss/agent/EnumUtils.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {

/*
 * LedManager ctor()
 *
 * LedManager constructor will create PubSub manager to subscribe to the
 * SwitchState update on FSDB. The switch state info is pushed by callback from
 * FsdbSwitchStateSubscriber and the updates used here
 */
LedManager::LedManager() {
  // Create Event base
  eventBase_ = std::make_unique<folly::EventBase>();
  auto* evb = eventBase_.get();
  ledManagerThread_ =
      std::make_unique<std::thread>([=] { evb->loopForever(); });

  // Read the LED config from file
  try {
    ledConfig_ = LedConfig::fromDefaultFile();
  } catch (FbossError& e) {
    XLOG(ERR) << "LED config file not present: " << e.what();
  }

  // Subscribe to Fsdb
  fsdbPubSubMgr_ = std::make_unique<fsdb::FsdbPubSubManager>("led_service");
  fsdbSwitchStateSubscriber_ =
      std::make_unique<FsdbSwitchStateSubscriber>(fsdbPubSubMgr_.get());
  fsdbSwitchStateSubscriber_->subscribeToSwitchState(this);

  XLOG(INFO) << "Created base LED Manager and subscribed to FSDB";
}

/*
 * LedManager destructor()
 *
 * Stop event base thread for Led Manager
 */
LedManager::~LedManager() {
  fsdbSwitchStateSubscriber_->removeSwitchStateSubscription();
  XLOG(INFO) << "Removed LED manager subscription from FSDB";

  if (eventBase_) {
    eventBase_->terminateLoopSoon();
  }
  if (ledManagerThread_) {
    ledManagerThread_->join();
  }
  XLOG(INFO)
      << "Removed LED manager thread and deleting Led Manager object now";
}

/*
 * updateLedStatus
 *
 * This function does these two things:
 * 1. Look into the FSDB pushed data (switch states) in newSwitchState and
 *    updates the local structure portDisplayMap_ as per the operational values
 *    in latest switch state
 * 2. Compute LED colors for new state and if the color for a port is different
 *    than existing one then set the new color on LED
 */
void LedManager::updateLedStatus(
    const std::map<short, LedSwitchStateUpdate>& newSwitchState) {
  if (newSwitchState.empty()) {
    // No change in port info so return from here
    return;
  }

  for (const auto& [portId, switchStateUpdate] : newSwitchState) {
    // Step 1. Update all operational info
    auto portName = switchStateUpdate.portName;
    auto portProfile = switchStateUpdate.portProfile;
    auto portProfileEnumVal = nameToEnum<cfg::PortProfileID>(portProfile);
    PortDisplayInfo portInfo;
    portInfo.portName = portName;
    portInfo.portProfileId = portProfileEnumVal;
    portInfo.operationStateUp = switchStateUpdate.operState;
    if (switchStateUpdate.ledExternalState.has_value()) {
      portInfo.cablingError = switchStateUpdate.ledExternalState.value() ==
              PortLedExternalState::CABLING_ERROR ||
          switchStateUpdate.ledExternalState.value() ==
              PortLedExternalState::CABLING_ERROR_LOOP_DETECTED;
    }
    if (portDisplayMap_.find(portId) != portDisplayMap_.end()) {
      // If the port info exists then carry the current color and port forced
      // LED info
      portInfo.currentLedState = portDisplayMap_.at(portId).currentLedState;
      portInfo.forcedOn = portDisplayMap_.at(portId).forcedOn;
      portInfo.forcedOff = portDisplayMap_.at(portId).forcedOff;
    } else {
      portInfo.currentLedState = utility::constructLedState(
          led::LedColor::UNKNOWN, led::Blink::UNKNOWN);
    }
    portInfo.activeState = switchStateUpdate.activeState;
    portInfo.drained = switchStateUpdate.drained;

    portDisplayMap_[portId] = portInfo;
  }

  for (const auto& [portId, switchStateUpdate] : newSwitchState) {
    // Step 2. Update LED color if required

    // If the LED state is forced by user then don't change LED color
    if ((portDisplayMap_.find(portId) != portDisplayMap_.end()) &&
        (portDisplayMap_[portId].forcedOn ||
         portDisplayMap_[portId].forcedOff)) {
      continue;
    }

    auto portName = switchStateUpdate.portName;
    auto portProfile = switchStateUpdate.portProfile;
    auto portProfileEnumVal = nameToEnum<cfg::PortProfileID>(portProfile);
    try {
      auto newLedState = calculateLedState(portId, portProfileEnumVal);
      if (newLedState != portDisplayMap_[portId].currentLedState) {
        setLedState(portId, portProfileEnumVal, newLedState);
        portDisplayMap_[portId].currentLedState = newLedState;
        XLOG(DBG2) << folly::sformat(
            "Port {:s} LED color changed to {:s}, Blink changed to {:s}",
            portName,
            enumToName<led::LedColor>(newLedState.ledColor().value()),
            enumToName<led::Blink>(newLedState.blink().value()));
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Failed to update LED color for port " << portName << ": "
                << ex.what();
    }
  }
}

/*
 * setExternalLedState
 *
 * This function sets the LED forced mode (on/off) as told through thrift call.
 * This function will also set the led color accordingly. The regular FSDB
 * based update function will treat the user forced on/off as higher priority
 * and they will not override it based on current port status
 */
void LedManager::setExternalLedState(
    int32_t portNum,
    PortLedExternalState ledState) {
  // Set the Forced on/off values in portDisplayMap_
  if (portDisplayMap_.find(portNum) == portDisplayMap_.end()) {
    // If the PortInfo has not been updated from FSDB yet then the important
    // info like port profile is not available so we need to bail out from
    // this functon
    throw FbossError(
        folly::sformat(
            "setExternalLedState: Port info not available for {:d} yet",
            portNum));
  } else {
    portDisplayMap_[portNum].forcedOn =
        ledState == PortLedExternalState::EXTERNAL_FORCE_ON;
    portDisplayMap_[portNum].forcedOff =
        ledState == PortLedExternalState::EXTERNAL_FORCE_OFF;
  }

  // Step 2. Update LED color if required
  auto portName = portDisplayMap_[portNum].portName;
  auto portProfile = portDisplayMap_[portNum].portProfileId;
  auto newLedState = calculateLedState(portNum, portProfile);
  if (newLedState != portDisplayMap_[portNum].currentLedState) {
    setLedState(portNum, portProfile, newLedState);
    portDisplayMap_[portNum].currentLedState = newLedState;
    XLOG(DBG2) << folly::sformat(
        "Port {:s} LED color changed to {:s}, Blink changed to {:s}",
        portName,
        enumToName<led::LedColor>(newLedState.ledColor().value()),
        enumToName<led::Blink>(newLedState.blink().value()));
  }
}

/*
 * isLedControlledThroughService
 *
 * Returns if the LED is being controlled through LED Service
 * - If /etc/coop/led.conf does not exist then return false
 * - If the led.conf file exist then return the value of boolean
 *   ledControlledThroughService from that file
 */
bool LedManager::isLedControlledThroughService() const {
  if (ledConfig_.get()) {
    return ledConfig_->thriftConfig_.ledControlledThroughService().value();
  }
  return false;
}

/*
 * getLedColorForPort
 *
 * Returns the latest LED color for a given SW Port
 */
led::LedColor LedManager::getCurrentLedColor(int32_t portNum) const {
  led::LedColor ledColor = led::LedColor::UNKNOWN;

  if (portDisplayMap_.find(portNum) != portDisplayMap_.end()) {
    ledColor = portDisplayMap_.at(portNum).currentLedState.ledColor().value();
  } else {
    auto portName = platformMapping_->getPortNameByPortId(PortID(portNum));
    XLOG(ERR) << folly::sformat(
        "Port {:s} not found in portDisplayMap_",
        (portName.has_value() ? portName.value() : ""));
  }

  return ledColor;
}

/*
 * getPortLedState
 *
 * Returns the LED state information for a given SW port. It reads from local
 * LED cache and throws if the cache is not updated yet from FSDB. The function
 * expects to be called under Led Manager event base.
 */
led::PortLedState LedManager::getPortLedState(
    const std::string& swPortName) const {
  led::PortLedState portLedState;

  auto portId = platformMapping_->getPortID(swPortName);
  if (portDisplayMap_.find(portId) == portDisplayMap_.end()) {
    // If the PortInfo has not been updated from FSDB yet
    throw FbossError(
        folly::sformat(
            "getPortLedState: Port info not available for {:s} yet",
            swPortName));
  }
  led::LedState ledState = portDisplayMap_.at(portId).currentLedState;

  portLedState.swPortId() = portId;
  portLedState.swPortName() = swPortName;
  portLedState.currentLedState() = ledState;
  portLedState.forcedOnState() = portDisplayMap_.at(portId).forcedOn;
  portLedState.forcedOffState() = portDisplayMap_.at(portId).forcedOff;

  return portLedState;
}

} // namespace facebook::fboss
