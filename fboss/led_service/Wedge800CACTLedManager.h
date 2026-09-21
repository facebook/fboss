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
 * Wedge800CACTLedManager class definiton:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Wedge800CACTLedManager : public BspLedManager {
 public:
  Wedge800CACTLedManager();
  virtual ~Wedge800CACTLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Wedge800CACTLedManager(Wedge800CACTLedManager const&) = delete;
  Wedge800CACTLedManager& operator=(Wedge800CACTLedManager const&) = delete;
};

} // namespace facebook::fboss
