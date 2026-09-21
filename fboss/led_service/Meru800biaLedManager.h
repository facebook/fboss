// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/led_service/BspLedManager.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook::fboss {

/*
 * Meru800biaLedManager class definiton:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Meru800biaLedManager : public BspLedManager {
 public:
  Meru800biaLedManager();
  virtual ~Meru800biaLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Meru800biaLedManager(Meru800biaLedManager const&) = delete;
  Meru800biaLedManager& operator=(Meru800biaLedManager const&) = delete;
};

} // namespace facebook::fboss
