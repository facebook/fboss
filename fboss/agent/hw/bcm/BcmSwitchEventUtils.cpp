/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmSwitchEventUtils.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmSwitchEventCallback.h"

namespace facebook::fboss {

namespace BcmSwitchEventUtils {

// Functor to fix a quirk in pre-c++14 standards
struct EnumClassHash {
  template <typename T>
  std::size_t operator()(T t) const {
    return static_cast<std::size_t>(t);
  }
};

// Map to store all the callbacks
using PerUnitCallbackMap = std::unordered_map<
    bcm_switch_event_t,
    std::shared_ptr<BcmSwitchEventCallback>,
    EnumClassHash>;
using CallbackMap = std::unordered_map<int, PerUnitCallbackMap>;
folly::Synchronized<CallbackMap> callbacks;

// Registers a callback with the BCM library to start callbacks
void initUnit(const int unit, BcmSwitch* bcmSwitch) {
  SYNCHRONIZED(callbacks) {
    if (callbacks.count(unit) > 0) {
      auto rv = bcm_switch_event_unregister(unit, callbackDispatch, nullptr);
      bcmCheckError(rv, "failed to unregister switch event");
    }

    // Add a new CallbackMap
    callbacks[unit].clear();
    auto rv = bcm_switch_event_register(unit, callbackDispatch, bcmSwitch);
    bcmCheckError(rv, "failed to register switch event");
  }
}

// Unregisters the callback and destroys existing callbacks
void resetUnit(const int unit) {
  SYNCHRONIZED(callbacks) {
    if (callbacks.count(unit) == 0) {
      return;
    }

    callbacks.erase(unit);
    auto rv = bcm_switch_event_unregister(unit, callbackDispatch, nullptr);
    bcmCheckError(rv, "failed to unregister switch event");
  }
}

// register a switch event callback. only one callback is allowed per switch
// event (no callback chaining). default behavior is to log and ignore. this
// is to prevent unexpected behavioral changes to existing code.
//
// returns: std::shared_ptr to the old callback object
//          nullptr if no callback for this event exists
std::shared_ptr<BcmSwitchEventCallback> registerSwitchEventCallback(
    const int unit,
    bcm_switch_event_t eventID,
    std::shared_ptr<BcmSwitchEventCallback> callback) {
  std::shared_ptr<BcmSwitchEventCallback> result;
  SYNCHRONIZED(callbacks) {
    auto unitCallbacks = callbacks.find(unit);
    if (unitCallbacks != callbacks.end()) {
      auto ret = unitCallbacks->second.emplace(eventID, callback);
      if (!ret.second) {
        result = ret.first->second;
        ret.first->second = callback;
      }
    }
  }

  return result;
}

// unregisters a switch event callback. reverts the handling of this switch
// event to the default behavior, which is to log and ignore.
void unregisterSwitchEventCallback(
    const int unit,
    const bcm_switch_event_t eventID) {
  SYNCHRONIZED(callbacks) {
    auto unitCallbacks = callbacks.find(unit);
    if (unitCallbacks != callbacks.end()) {
      unitCallbacks->second.erase(eventID);
    }
  }
}

// central location for all switch event callbacks
void callbackDispatch(
    int unit,
    bcm_switch_event_t eventID,
    uint32_t arg1,
    uint32_t arg2,
    uint32_t arg3,
    void* data) {
  std::shared_ptr<BcmSwitchEventCallback> callbackObj;
  SYNCHRONIZED_CONST(callbacks) {
    auto unitCallbacks = callbacks.find(unit);
    if (unitCallbacks != callbacks.end()) {
      auto iterator = unitCallbacks->second.find(eventID);
      if (iterator != unitCallbacks->second.end()) {
        callbackObj = iterator->second;
      }
    }
  }

  // perform user-specified callback if it exists
  if (callbackObj) {
    callbackObj->callback(unit, eventID, arg1, arg2, arg3, data);
    // no callback found -- use default callback
  } else {
    defaultCallback(unit, eventID, arg1, arg2, arg3);
  }
}

std::string getAlarmName(const bcm_switch_event_t eventID) {
  switch (eventID) {
    case BCM_SWITCH_EVENT_STABLE_FULL:
      return "BCM_SWITCH_EVENT_STABLE_FULL";
    case BCM_SWITCH_EVENT_STABLE_ERROR:
      return "BCM_SWITCH_EVENT_STABLE_ERROR";
    case BCM_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN:
      return "BCM_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN";
    case BCM_SWITCH_EVENT_WARM_BOOT_DOWNGRADE:
      return "BCM_SWITCH_EVENT_WARM_BOOT_DOWNGRADE";
    case BCM_SWITCH_EVENT_PARITY_ERROR:
      return "BCM_SWITCH_EVENT_PARITY_ERROR";
    case BCM_SWITCH_EVENT_MMU_BST_TRIGGER:
      return "BCM_SWITCH_EVENT_MMU_BST_TRIGGER";
    default:
      return "UNKNOWN";
  }
}

// default switch event behavior -- log warning and return
// to preserve original system behavior, don't terminate the program
void defaultCallback(
    const int unit,
    const bcm_switch_event_t eventID,
    const uint32_t arg1,
    const uint32_t arg2,
    const uint32_t arg3) {
  std::string alarm = getAlarmName(eventID);
  XLOG(ERR) << "BCM Unhandled switch event " << alarm << "(" << eventID
            << ") on hw unit " << unit << " with params: (" << arg1 << ", "
            << arg2 << ", " << arg3 << ")";
}

void exportEventCounters(
    const bcm_switch_event_t eventID,
    bool fatal,
    void* data) {
  BcmSwitch* bcmSwitch = reinterpret_cast<BcmSwitch*>(data);
  if (eventID != BCM_SWITCH_EVENT_PARITY_ERROR) {
    bcmSwitch->getSwitchStats()->asicError();
  } else {
    if (fatal) {
      bcmSwitch->getSwitchStats()->uncorrParityError();
    } else {
      bcmSwitch->getSwitchStats()->corrParityError();
    }
  }
}

} // namespace BcmSwitchEventUtils

} // namespace facebook::fboss
