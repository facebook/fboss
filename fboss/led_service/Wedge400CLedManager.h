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
 * Wedge400CLedManager class definiton:
 *
 * The Wedge400CLedManager class managing all Port LED in the Wedge400C system.
 * This platform specific class is responsibe for the platform specific work
 * like FPGA operation, setting platform specific color of LED. For rest of the
 * work the base class is used
 */
class Wedge400CLedManager : public Wedge400BaseLedManager {
 public:
  Wedge400CLedManager();
  virtual ~Wedge400CLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Wedge400CLedManager(Wedge400CLedManager const&) = delete;
  Wedge400CLedManager& operator=(Wedge400CLedManager const&) = delete;
};

} // namespace facebook::fboss
