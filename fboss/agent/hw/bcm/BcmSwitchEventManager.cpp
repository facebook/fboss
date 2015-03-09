/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmSwitchEventManager.h"
#include "BcmSwitch.h"
#include "BcmSwitchEvent.h"
#include "BcmSwitchEventCallback.h"

namespace facebook { namespace fboss {

// constructor registers itself with the Bcm library to start callbacks
BcmSwitchEventManager::BcmSwitchEventManager(BcmSwitch* hw)
  :hw_(hw) {

  opennsl_switch_event_register(hw->getUnit(), callbackDispatch, this);
}

// destructor for BcmSwitchEventManager
BcmSwitchEventManager::~BcmSwitchEventManager() {
  std::lock_guard<std::mutex> g(lock_);
  callbacks_.clear();
}

// register a switch event callback. only one callback is allowed per switch
// event (no callback chaining). default behavior is to log and ignore. this
// is to prevent unexpected behavioral changes to existing code.
//
// returns: std::shared_ptr to the old callback object
//          nullptr if no callback for this event exists
std::shared_ptr<BcmSwitchEventCallback> BcmSwitchEventManager::
  registerSwitchEventCallback(opennsl_switch_event_t eventID,
  std::shared_ptr<BcmSwitchEventCallback> callback) {

  std::shared_ptr<BcmSwitchEventCallback> result;
  std::lock_guard<std::mutex> g(lock_);
  auto ret = callbacks_.emplace(eventID, callback);
  if (!ret.second) {
    result = ret.first->second;
    ret.first->second = callback;
  }
  return result;
}

// unregisters a switch event callback. reverts the handling of this switch
// event to the default behavior, which is to log and ignore.
void BcmSwitchEventManager::unregisterSwitchEventCallback(
  opennsl_switch_event_t eventID) {

  std::lock_guard<std::mutex> g(lock_);
  callbacks_.erase(eventID);
}

// central location for all switch event callbacks
void BcmSwitchEventManager::callbackDispatch(int unit,
  opennsl_switch_event_t eventID,
  uint32_t alarmID,
  uint32_t portID,
  uint32_t raised,
  void* userdata) {

  BcmSwitchEventManager* instance =
    static_cast<BcmSwitchEventManager*>(userdata);
  BcmSwitchEvent event(instance->hw_, unit, eventID, alarmID, portID,
    raised > 0);

  std::shared_ptr<BcmSwitchEventCallback> callbackObj;
  {
    std::lock_guard<std::mutex> g(instance->lock_);
    auto iterator = instance->callbacks_.find(eventID);
    if (iterator != instance->callbacks_.end()) {
      callbackObj = iterator->second;
    }
  }

  // perform user-specified callback if it exists
  if (callbackObj) {
    callbackObj->callback(event);

  // no callback found -- use default callback
  } else {
    instance->defaultCallback(event);
  }
}

// default switch event behavior -- log warning and return
// to preserve original system behavior, don't terminate the program
// TODO (#4594034) -- translate numeric event/alarm codes into verbose info.
void BcmSwitchEventManager::defaultCallback(const BcmSwitchEvent& event) {

  LOG(WARNING) << "critical switch event "
    << (event.eventRaised() ? "triggered" : "cleared")
    << " on hw unit " << event.getUnit()
    << ". event code " << event.getEventID()
    << " alarm ID " << event.getAlarmID()
    << " port ID " << event.getPortID() << ".";
}

}}
