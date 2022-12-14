// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <string.h>

#include "fboss/platform/fbdevd/FruManager.h"
#include "fboss/platform/fbdevd/I2cController.h"

namespace facebook::fboss::platform::fbdevd {

FruManager::FruManager(const FruModule& fru) {
  // TODO: store FruModule(thrift) structure for future reference.

  // Process FRU hotplug event for the first time.
  processEvents();
}

bool FruManager::isPresent() {
  // TODO: determine if the FRU is plugged/removed by reading FruModule
  // "fruState" config.
  return true;
}

void FruManager::processEvents() {
  bool curState = isPresent();

  if (curState != isPresent_) {
    if (curState) {
      // Handle newly-plugged FRU
      // TODO:
      //   1. trigger FruModule.initOps
      //   2. create i2c client devices
    } else {
      // Handle FRU removal event
      // TODO:
      //   1. remove i2c client devices
      //   2. trigger FruModule.cleanupOps
    }

    isPresent_ = curState;
  }
}
} // namespace facebook::fboss::platform::fbdevd
