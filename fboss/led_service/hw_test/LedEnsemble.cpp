/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/led_service/hw_test/LedEnsemble.h"
#include "fboss/agent/FbossError.h"
#include "fboss/led_service/LedManagerInit.h"
#include "fboss/lib/CommonFileUtils.h"

namespace facebook::fboss {

void LedEnsemble::init() {
  ledManager_ = createLedManager();
  if (!ledManager_.get()) {
    throw FbossError("LedEnsemble: Failed to create Led Manager");
  }
}

LedManager* LedEnsemble::getLedManager() {
  return ledManager_.get();
}

} // namespace facebook::fboss
