// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/led_service/BspLedManager.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook::fboss {

/*
 * Morgan800ccLedManager class definiton:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class Morgan800ccLedManager : public BspLedManager {
 public:
  Morgan800ccLedManager();
  virtual ~Morgan800ccLedManager() override {}

  // Forbidden copy constructor and assignment operator
  Morgan800ccLedManager(Morgan800ccLedManager const&) = delete;
  Morgan800ccLedManager& operator=(Morgan800ccLedManager const&) = delete;
};

} // namespace facebook::fboss
