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

#include <folly/Format.h>
#include <folly/Synchronized.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <stdexcept>
#include "fboss/agent/FbossError.h"
#include "fboss/led_service/LedManager.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook::fboss {

/*
 * BspLedManager class definiton:
 *
 * The BspLedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class BspLedManager : public LedManager {
 public:
  BspLedManager();
  virtual ~BspLedManager() override;

  // Forbidden copy constructor and assignment operator
  BspLedManager(BspLedManager const&) = delete;
  BspLedManager& operator=(BspLedManager const&) = delete;

 protected:
  // System container to get LED controller
  BspSystemContainer* bspSystemContainer_{nullptr};

  virtual led::LedColor calculateLedColor(
      uint32_t portId,
      cfg::PortProfileID portProfile) const override;

  virtual void setLedColor(
      uint32_t portId,
      cfg::PortProfileID portProfile,
      led::LedColor ledColor) override;

  led::LedColor getLedColorFromPortStatus(
      bool anyPortUp,
      bool allPortsUp,
      bool allPortsReachable) const;

  std::set<int> getLedIdFromSwPort(
      uint32_t portId,
      cfg::PortProfileID portProfile) const;

  std::vector<uint32_t> getCommonLedSwPorts(
      uint32_t portId,
      cfg::PortProfileID portProfile) const;
};

} // namespace facebook::fboss
