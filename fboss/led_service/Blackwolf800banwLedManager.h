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
 * Blackwolf800banwLedManager class definition:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Blackwolf800banwLedManager : public BspLedManager {
 public:
  Blackwolf800banwLedManager();
  virtual ~Blackwolf800banwLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Blackwolf800banwLedManager(Blackwolf800banwLedManager const&) = delete;
  Blackwolf800banwLedManager& operator=(Blackwolf800banwLedManager const&) =
      delete;
};

} // namespace facebook::fboss
