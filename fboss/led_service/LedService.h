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
      std::map<short, LedManager::LedSwitchStateUpdate>& newSwitchState) {
    pLedManager_->updateLedStatus(newSwitchState);
  }

  void setExternalLedState(int32_t portNum, PortLedExternalState ledState) {
    auto evb = pLedManager_->getEventBase();
    if (!evb) {
      throw FbossError("Event base not available for Led Manager");
    }
    folly::via(evb).thenValue(
        [=](auto&&) { pLedManager_->setExternalLedState(portNum, ledState); });
  }

  led::LedState getLedState(const std::string& swPortName) {
    auto evb = pLedManager_->getEventBase();
    if (!evb) {
      throw FbossError("Event base not available for Led Manager");
    }
    return folly::via(evb)
        .thenValue(
            [=](auto&&) { return pLedManager_->getLedState(swPortName); })
        .get();
  }

 private:
  // LED manager object
  std::unique_ptr<LedManager> pLedManager_;
};
} // namespace facebook::fboss
