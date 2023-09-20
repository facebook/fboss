// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/led_service/BspLedManager.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook::fboss {

/*
 * Meru800bfaLedManager class definiton:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Meru800bfaLedManager : public BspLedManager {
 public:
  Meru800bfaLedManager();
  virtual ~Meru800bfaLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Meru800bfaLedManager(Meru800bfaLedManager const&) = delete;
  Meru800bfaLedManager& operator=(Meru800bfaLedManager const&) = delete;
};

} // namespace facebook::fboss
