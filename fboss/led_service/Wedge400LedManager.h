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

#include "fboss/led_service/Wedge400BaseLedManager.h"

namespace facebook::fboss {

/*
 * Wedge400LedManager class definiton:
 *
 * The Wedge400LedManager class managing all Port LED in the Wedge400 system.
 * This platform specific class is responsibe for the platform specific work
 * like FPGA operation, setting platform specific color of LED. For rest of the
 * work the base class is used
 */
class Wedge400LedManager : public Wedge400BaseLedManager {
 public:
  Wedge400LedManager();
  virtual ~Wedge400LedManager() override {}

  // Forbidden copy constructor and assignment operator
  Wedge400LedManager(Wedge400LedManager const&) = delete;
  Wedge400LedManager& operator=(Wedge400LedManager const&) = delete;
};

} // namespace facebook::fboss
