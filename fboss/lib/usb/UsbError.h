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
#include <string>
#include <stdexcept>

namespace facebook { namespace fboss {

class UsbError : public std::exception {
 public:
  template<typename... Args>
  explicit UsbError(const Args&... args)
    : what_(folly::to<std::string>(args...)) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

 private:
  std::string what_;
};

class LibusbError : public UsbError {
 public:
  template<typename... Args>
  explicit LibusbError(int error, const Args&... args)
    : UsbError(args..., ": ", libusb_error_name(error)),
      error_(error) {}

  int errorCode() const {
    return error_;
  }

 private:
  int error_;
};

}} // facebook::fboss
