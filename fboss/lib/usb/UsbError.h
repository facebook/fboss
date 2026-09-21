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

#include <folly/Conv.h>

#include <libusb-1.0/libusb.h>
#include <stdexcept>
#include <string>

#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook::fboss {
class UsbError : public I2cError {
 public:
  template <typename... Args>
  explicit UsbError(const Args&... args)
      : I2cError(folly::to<std::string>(args...)) {}
};

class LibusbError : public UsbError {
 public:
  template <typename... Args>
  explicit LibusbError(int error, const Args&... args)
      : UsbError(args..., ": ", libusb_error_name(error)), error_(error) {}

  int errorCode() const {
    return error_;
  }

 private:
  int error_;
};

class UsbDeviceResetError : public UsbError {
 public:
  template <typename... Args>
  explicit UsbDeviceResetError(const Args&... args) : UsbError(args...) {}
};

} // namespace facebook::fboss
