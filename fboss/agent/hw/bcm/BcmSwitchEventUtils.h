/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <memory>
#include <mutex>

#include <folly/Synchronized.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmSwitchEventCallback.h"

extern "C" {
#include <bcm/switch.h>
}

class BcmSwitch;

namespace facebook::fboss {

/**
 * Utility class to manage the BCM callbacks for critical switch events and
 * invoke the callback on the switch event handler objects.
 */
namespace BcmSwitchEventUtils {
// Initializes/resets event handling for a unit.  This will register or
// unregister a callback with the Bcm SDK and allocate/deallocate our
// local state.
void initUnit(const int unit, BcmSwitch* bcmSwitch);
void resetUnit(const int unit);

// Callback registration.
// Registers a new event callback, returning the old callback object if it
// exists. Callbacks are not chained.
std::shared_ptr<BcmSwitchEventCallback> registerSwitchEventCallback(
    const int unit,
    const bcm_switch_event_t eventID,
    std::shared_ptr<BcmSwitchEventCallback> callback);

// removes all callbacks for a given switch event
void unregisterSwitchEventCallback(
    const int unit,
    const bcm_switch_event_t eventID);

// central callback for bcm library critical event callbacks (this
// function is registered with bcm_switch_event_register)
void callbackDispatch(
    int unit,
    bcm_switch_event_t eventID,
    uint32_t arg1,
    uint32_t arg2,
    uint32_t arg3,
    void* instance);

// Return the name of the alarm
std::string getAlarmName(const bcm_switch_event_t eventID);

// default callback for non-handled switch events.
void defaultCallback(
    const int unit,
    const bcm_switch_event_t eventID,
    const uint32_t arg1,
    const uint32_t arg2,
    const uint32_t arg3);

// Export counters
void exportEventCounters(
    const bcm_switch_event_t eventID,
    bool fatal,
    void* data);

} // namespace BcmSwitchEventUtils

} // namespace facebook::fboss
