/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/led_service/BspLedManager.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook::fboss {

/*
 * Minipack3BTALedManager class definiton:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Minipack3BTALedManager : public BspLedManager {
 public:
  Minipack3BTALedManager();
  virtual ~Minipack3BTALedManager() override {}

  // Forbidden copy constructor and assignment operator
  Minipack3BTALedManager(Minipack3BTALedManager const&) = delete;
  Minipack3BTALedManager& operator=(Minipack3BTALedManager const&) = delete;
};

} // namespace facebook::fboss
