/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>

#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/lib/usb/TransceiverPlatformApi.h"

namespace facebook::fboss {
/* This is class which inherits TransceiverPlatformApi class and contains the
 * TransceiverI2CApi object pointer. This class is used to return the generic
 * object which provides the API to control QSFP using FPGA based interface or
 * the I2C based interface while abstracting the Fpga or I2c based platform
 * knowledge inside
 */
class TransceiverPlatformI2cApi : public TransceiverPlatformApi {
 public:
  // Constructor gets TransceiverI2CApi object from caller and that object
  // is referred by this class for I2C based platform. For that platform the
  // Qsfp access control will be done by the functions implemented from that
  // stored raw pointer
  TransceiverPlatformI2cApi(TransceiverI2CApi* i2cBus) : i2cBus_(i2cBus) {}

  ~TransceiverPlatformI2cApi() {}

  // For platforms having i2c based qsfp control, the qsfp reset will be done
  // by the i2c object's qsfp reset implementation. The i2c object pointer is
  // populated to this class from constructor
  void triggerQsfpHardReset(unsigned int module) override {
    i2cBus_->triggerQsfpHardReset(module);
  }

  // This function will bring all the transceivers out of reset, making use
  // of the specific implementation from each platform. Platforms that bring
  // transceiver out of reset by default will stay no op.
  void clearAllTransceiverReset() override {
    i2cBus_->clearAllTransceiverReset();
  }

  // For platforms having Qsfp control through I2C, the i2c object is referred
  // by this class. This is a raw pointer and its value is populated by this
  // class constructor
  TransceiverI2CApi* i2cBus_;
};

} // namespace facebook::fboss
