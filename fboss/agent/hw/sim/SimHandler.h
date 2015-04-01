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

#include "fboss/agent/hw/sim/gen-cpp2/SimCtrl.h"
#include "fboss/agent/ThriftHandler.h"

namespace facebook { namespace fboss {

class SimSwitch;
class SwSwitch;

class SimHandler : virtual public SimCtrlSvIf,
                   public ThriftHandler {
 public:
  SimHandler(SwSwitch* sw, SimSwitch* hw);

 private:
  // Forbidden copy constructor and assignment operator
  SimHandler(SimHandler const &) = delete;
  SimHandler& operator=(SimHandler const &) = delete;
};

}} // facebook::fboss
