/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmSwitchEvent.h"

namespace facebook { namespace fboss {

BcmSwitchEvent::BcmSwitchEvent(BcmSwitch* hw, int unit,
                               opennsl_switch_event_t eventID, uint32_t alarmID,
                               uint32_t portID, bool raised)
  : unit_(unit),
    eventID_(eventID),
    alarmID_(alarmID),
    portID_(portID),
    raised_(raised) {
}

}}
