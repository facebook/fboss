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

#include <stdint.h>

extern "C" {
#include <opennsl/switch.h>
}

namespace facebook { namespace fboss {

class BcmSwitch;

/**
 * A class for BCM switch events
 */

class BcmSwitchEvent {
 public:
  BcmSwitchEvent(BcmSwitch* hw, int unit, opennsl_switch_event_t eventID,
    uint32_t alarmID, uint32_t portID, bool raised);

  /*
   * Getters.
   */
  opennsl_switch_event_t getEventID() const {
    return eventID_;
  }
  int getUnit() const {
    return unit_;
  }
  uint32_t getAlarmID() const {
    return alarmID_;
  }
  uint32_t getPortID() const {
    return portID_;
  }
  bool eventRaised() const {
    return raised_;
  }

 private:
  int unit_;
  opennsl_switch_event_t eventID_;
  uint32_t alarmID_;
  uint32_t portID_;
  bool raised_;
};

}} // facebook::fboss
