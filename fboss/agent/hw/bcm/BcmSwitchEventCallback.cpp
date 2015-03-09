/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmSwitchEventCallback.h"
#include <glog/logging.h>
#include "BcmSwitchEvent.h"

namespace facebook { namespace fboss {

// check to make sure that the error was raised, not cleared. then log
// information as a fatal error and quit.
void BcmSwitchEventParityErrorCallback::callback(const BcmSwitchEvent& event) {

  if (event.eventRaised()) {
    LOG(FATAL) << "parity error on hw unit " << event.getUnit() << " logged.";

  } else {
    LOG(INFO) << "parity error on hw unit " << event.getUnit() << "cleared.";
  }

}

}}
