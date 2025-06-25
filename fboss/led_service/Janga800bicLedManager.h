// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/led_service/BspLedManager.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook::fboss {

/*
 * Janga800bicLedManager class definiton:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Janga800bicLedManager : public BspLedManager {
 public:
  Janga800bicLedManager();
  virtual ~Janga800bicLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Janga800bicLedManager(Janga800bicLedManager const&) = delete;
  Janga800bicLedManager& operator=(Janga800bicLedManager const&) = delete;
};

} // namespace facebook::fboss
