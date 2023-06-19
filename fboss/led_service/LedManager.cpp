// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/LedManager.h"
#include "fboss/agent/EnumUtils.h"
#include "fboss/agent/platforms/common/elbert/ElbertPlatformMapping.h"
#include "fboss/agent/platforms/common/fuji/FujiPlatformMapping.h"
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/CommonPortUtils.h"
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
  if (eventBase_) {
    eventBase_->terminateLoopSoon();
  }
  if (ledManagerThread_) {
    ledManagerThread_->join();
  }
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
    std::map<uint16_t, fboss::state::PortFields> newSwitchState) {
  if (newSwitchState.empty()) {
    // No change in port info so return from here
    return;
  }

  for (const auto& [portId, portFields] : newSwitchState) {
    // Step 1. Update all operational info
    auto portName = portFields.portName().value();
    auto portProfile = portFields.portProfileID().value();
    auto portProfileEnumVal = nameToEnum<cfg::PortProfileID>(portProfile);

    if (portDisplayMap_.find(portId) == portDisplayMap_.end()) {
      PortDisplayInfo portInfo;
      portInfo.portName = portName;
      portInfo.portProfileId = portProfileEnumVal;
      portInfo.operationStateUp = portFields.portOperState().value();
      portInfo.currentLedColor = led::LedColor::UNKNOWN;
      portDisplayMap_[portId] = portInfo;
    } else {
      portDisplayMap_[portId].portName = portName;
      portDisplayMap_[portId].portProfileId = portProfileEnumVal;
      portDisplayMap_[portId].operationStateUp =
          portFields.portOperState().value();
    }
  }

  for (const auto& [portId, portFields] : newSwitchState) {
    // Step 2. Update LED color if required
    auto portName = portFields.portName().value();
    auto portProfile = portFields.portProfileID().value();
    auto portProfileEnumVal = nameToEnum<cfg::PortProfileID>(portProfile);

    auto newLedColor = calculateLedColor(portId, portProfileEnumVal);
    if (newLedColor != portDisplayMap_[portId].currentLedColor) {
      setLedColor(portId, portProfileEnumVal, newLedColor);
      portDisplayMap_[portId].currentLedColor = newLedColor;
      XLOG(DBG2) << folly::sformat(
          "Port {:s} LED color changed to {:s}",
          portName,
          enumToName<led::LedColor>(newLedColor));
    }
  }
}

} // namespace facebook::fboss
