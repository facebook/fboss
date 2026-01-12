/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/rackmon/rack_firmware_update/FirmwareUpdater.h"
#include <chrono>
#include <thread>

namespace facebook::fboss::platform::rackmon {

FirmwareUpdater::FirmwareUpdater(
    std::shared_ptr<RackmonClient> client,
    uint16_t deviceAddr)
    : client_(std::move(client)), deviceAddr_(deviceAddr) {}

void FirmwareUpdater::sleep(uint32_t milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void FirmwareUpdater::sleepSeconds(uint32_t seconds) {
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

} // namespace facebook::fboss::platform::rackmon
