// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/led_service/BspLedManager.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook::fboss {

/*
 * Tahan800bcLedManager class definiton:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Tahan800bcLedManager : public BspLedManager {
 public:
  Tahan800bcLedManager();
  virtual ~Tahan800bcLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Tahan800bcLedManager(Tahan800bcLedManager const&) = delete;
  Tahan800bcLedManager& operator=(Tahan800bcLedManager const&) = delete;
};

} // namespace facebook::fboss
