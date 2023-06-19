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

#include "fboss/led_service/LedManager.h"

namespace facebook::fboss {

/*
 * LassenLedManager class definiton:
 *
 * The LassenLedManager class managing all Port LED in the Lassen system. This
 * platform specific class is responsibe for the platform specific work like
 * FPGA operation, setting platform specific color of LED. For rest of the work
 * the base class is used
 */
class LassenLedManager : public LedManager {
 public:
  LassenLedManager();
  virtual ~LassenLedManager() override {}

  // Forbidden copy constructor and assignment operator
  LassenLedManager(LassenLedManager const&) = delete;
  LassenLedManager& operator=(LassenLedManager const&) = delete;

 protected:
  virtual led::LedColor calculateLedColor(
      uint32_t portId,
      cfg::PortProfileID portProfile) const override;

  virtual void setLedColor(
      uint32_t portId,
      cfg::PortProfileID portProfile,
      led::LedColor ledColor) override;
};

} // namespace facebook::fboss
