// Copyright 2021- Facebook. All rights reserved.
#pragma once

#include <gflags/gflags.h>
#include <string>
#include "fboss/led_service/LedManager.h"

namespace folly {
class EventBase;
}

namespace facebook::fboss {

/*
 * LedService
 *
 * The main class for handling the Led Service. The service subscribes to the
 * FSDB for Switch State updates and then manages LED as per updates.
 * Related classes:
 *    LedManager - To manage the LED based on FSDB updates
 */

class LedService {
 public:
  // Constructor / destructor
  LedService() {}
  ~LedService() {}

  // Instantiates all data used by LED Service
  void kickStart();

  void updateLedStatus(
      std::map<uint16_t, fboss::state::PortFields> newSwitchState) {
    pLedManager_->updateLedStatus(newSwitchState);
  }

  void setExternalLedState(int32_t portNum, PortLedExternalState ledState) {
    pLedManager_->setExternalLedState(portNum, ledState);
  }

 private:
  // LED manager object
  std::unique_ptr<LedManager> pLedManager_;
};
} // namespace facebook::fboss
