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

struct libusb_device;
struct libusb_device_handle;

namespace facebook::fboss {
/*
 * This is a thin wrapper around libusb_device_handle.
 *
 * It mainly provides RAII cleanup on destruction, as well as some slighty
 * more native-feeling C++ methods.
 */
class UsbHandle {
 public:
  UsbHandle() {}
  explicit UsbHandle(libusb_device_handle* handle) : handle_(handle) {}
  ~UsbHandle() {
    close();
  }

  // Move operators
  UsbHandle(UsbHandle&& other) noexcept;
  UsbHandle& operator=(UsbHandle&& other) noexcept;

  libusb_device_handle* handle() {
    return handle_;
  }

  void openFrom(libusb_device* dev, bool autoDetach = true);
  void close();

  bool isOpen() const {
    return handle_ != nullptr;
  }

  void setAutoDetachKernelDriver(bool autoDetach);
  void claimInterface(int iface);

 private:
  UsbHandle(const UsbHandle& other) = delete;
  UsbHandle& operator=(UsbHandle& other) = delete;

  libusb_device_handle* handle_{nullptr};
};

} // namespace facebook::fboss
