/*
 *  Copyright (c) 2026 Nexthop Systems Inc.
 *  SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include "fboss/led_service/BspLedManager.h"

namespace facebook::fboss {

/*
 * M4062nhpLedManager class definition:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class M4062nhpLedManager : public BspLedManager {
 public:
  M4062nhpLedManager();
  ~M4062nhpLedManager() override = default;

  // Forbidden copy and move constructors and assignment operators
  M4062nhpLedManager(M4062nhpLedManager const&) = delete;
  M4062nhpLedManager& operator=(M4062nhpLedManager const&) = delete;
  M4062nhpLedManager(M4062nhpLedManager&&) = delete;
  M4062nhpLedManager& operator=(M4062nhpLedManager&&) = delete;
};

} // namespace facebook::fboss
