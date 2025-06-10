/*
 *  Copyright (c) 2025-present, Facebook, Inc.
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
 * Glath05a-64oLedManager class definiton:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Glath05a_64oLedManager : public BspLedManager {
 public:
  Glath05a_64oLedManager();
  virtual ~Glath05a_64oLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Glath05a_64oLedManager(Glath05a_64oLedManager const&) = delete;
  Glath05a_64oLedManager& operator=(Glath05a_64oLedManager const&) = delete;
};

} // namespace facebook::fboss
