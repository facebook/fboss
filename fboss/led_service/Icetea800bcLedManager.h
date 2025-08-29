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
 * Icetea800bcLedManager class definition:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Icetea800bcLedManager : public BspLedManager {
 public:
  Icetea800bcLedManager();
  virtual ~Icetea800bcLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Icetea800bcLedManager(Icetea800bcLedManager const&) = delete;
  Icetea800bcLedManager& operator=(Icetea800bcLedManager const&) = delete;
};

} // namespace facebook::fboss
