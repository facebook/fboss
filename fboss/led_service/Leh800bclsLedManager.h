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
 * Leh800bclsLedManager class definition:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Leh800bclsLedManager : public BspLedManager {
 public:
  Leh800bclsLedManager();
  virtual ~Leh800bclsLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Leh800bclsLedManager(Leh800bclsLedManager const&) = delete;
  Leh800bclsLedManager& operator=(Leh800bclsLedManager const&) = delete;
};

} // namespace facebook::fboss
