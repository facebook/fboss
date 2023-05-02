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
#include <folly/logging/xlog.h>
#include <stdexcept>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/bsp/BspSystemContainer.h"
#include "fboss/lib/led/LedIO.h"
#include "fboss/lib/led/gen-cpp2/led_mapping_types.h"

namespace facebook::fboss {

/*
 * LedManager class definiton:
 *
 * The LedManager class managing all LED in the system. The object is spawned
 * by LED Service. This will subscribe to Fsdb to get Switch state update and
 * then update the LED in hardware
 */
class LedManager {
  using PortDisplayInfo = struct PortDisplayInfo {
    std::string portName;
    cfg::PortProfileID portProfileId;
    bool operationStateUp{false};
    bool neighborReachable{false};
    led::LedColor currentLedColor{led::LedColor::UNKNOWN};
  };

 public:
  LedManager() {}
  virtual ~LedManager() {}

  // Initialize the Led Manager, get system container
  virtual void initLedManager() {}

  // On getting the update from FSDB, update portDisplayList_
  void updateLedStatus() {}

  // Forbidden copy constructor and assignment operator
  LedManager(LedManager const&) = delete;
  LedManager& operator=(LedManager const&) = delete;

  folly::Synchronized<std::map<uint16_t, fboss::state::PortFields>>
      subscribedAgentSwitchStatePortData_;

 protected:
  // System container to get LED controller
  BspSystemContainer* bspSystemContainer_{nullptr};

  // Platform mapping to get SW Port -> Lane mapping
  std::unique_ptr<PlatformMapping> platformMapping_;

  // Port Name to PortDisplayInfo map, no lock needed
  std::map<uint32_t, PortDisplayInfo> portDisplayList_;

  virtual led::LedColor calculateLedColor(
      uint32_t portId,
      cfg::PortProfileID portProfile) {
    return led::LedColor::UNKNOWN;
  }
  virtual void setLedColor(uint32_t portId, led::LedColor ledColor) {}

  std::vector<int> getLedIdFromSwPort(
      uint32_t portId,
      cfg::PortProfileID portProfile) const;
};

} // namespace facebook::fboss
