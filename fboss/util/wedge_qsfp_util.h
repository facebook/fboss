// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/lib/usb/TransceiverPlatformApi.h"
#include "fboss/lib/usb/TransceiverPlatformI2cApi.h"

#include <memory>
#include <utility>

std::pair<std::unique_ptr<facebook::fboss::TransceiverI2CApi>, int>
getTransceiverAPI();

/* This function creates and returns TransceiverPlatformApi object. For Fpga
 * controlled platform this function creates Platform specific TransceiverApi
 * object and returns it. For I2c controlled platform this function creates
 * TransceiverPlatformI2cApi object and keeps the platform specific I2CBus
 * object raw pointer inside it for reference. The returned object's Qsfp
 * control function is called here to use appropriate Fpga/I2c Api in this
 * function.
 */
std::pair<std::unique_ptr<facebook::fboss::TransceiverPlatformApi>, int>
  getTransceiverPlatformAPI(facebook::fboss::TransceiverI2CApi *i2cBus);

bool isTrident2();
