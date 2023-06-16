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

#include "fboss/led_service/MinipackBaseLedManager.h"

namespace facebook::fboss {

/*
 * FujiLedManager class definiton:
 *
 * The FujiLedManager is derived from MinipackLedManager class for managing
 * all Port LED in the Fuji system. This class will use same code as Minipack
 * except for setting of LED color in HW
 */
class FujiLedManager : public MinipackBaseLedManager {
 public:
  FujiLedManager();
  virtual ~FujiLedManager() override;

  // Forbidden copy constructor and assignment operator
  FujiLedManager(FujiLedManager const&) = delete;
  FujiLedManager& operator=(FujiLedManager const&) = delete;

 protected:
  virtual void setLedColor(
      uint32_t portId,
      cfg::PortProfileID portProfile,
      led::LedColor ledColor) override;
};

} // namespace facebook::fboss
