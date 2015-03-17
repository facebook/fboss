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
#include "fboss/agent/FbossError.h"

extern "C" {
#include <opennsl/switch.h>
}

namespace facebook { namespace fboss {

class BcmSwitch;
class BcmSwitchEvent;
class BcmSwitchEventCallback;

/**
 * This class manages the bcm callbacks for critical switch events and
 * invokes the callback on the switch event handler objects.
 */

class BcmSwitchEventManager {
 public:
  explicit BcmSwitchEventManager(BcmSwitch* hw);
  ~BcmSwitchEventManager();

  // callback registration.
  // registers a new event callback, returning the old callback object if
  // it exists. callbacks are not chained
  std::shared_ptr<BcmSwitchEventCallback> registerSwitchEventCallback(
    opennsl_switch_event_t eventID,
    std::shared_ptr<BcmSwitchEventCallback> callback);

  // removes all callbacks for a given switch event
  void unregisterSwitchEventCallback(opennsl_switch_event_t eventID);

 private:
  BcmSwitch* hw_;
  std::mutex lock_;
  std::map<opennsl_switch_event_t,
    std::shared_ptr<BcmSwitchEventCallback>> callbacks_;

  // disable copy constructor and assignment operator
  BcmSwitchEventManager(const BcmSwitchEventManager&) = delete;
  BcmSwitchEventManager& operator=(const BcmSwitchEventManager&) = delete;

  // central callback for bcm library critical event callbacks (this function
  // is registered with bcm_switch_event_register)
  static void callbackDispatch(int unit, opennsl_switch_event_t event,
    uint32_t alarmID, uint32_t portID, uint32_t raised, void* instance);

  // default callback for non-handled switch events.
  void defaultCallback(const BcmSwitchEvent& event);

};

}} // facebook::fboss
