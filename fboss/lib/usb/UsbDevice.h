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

struct libusb_context;
struct libusb_device;

namespace facebook::fboss {
class UsbHandle;

/*
 * This is a thin wrapper around libusb_device.
 *
 * It mainly provides RAII cleanup on destruction, as well as some slighty
 * more native-feeling C++ methods.
 */
class UsbDevice {
 public:
  UsbDevice() {}
  /*
   * Create a new UsbDevice from a libusb_device.
   *
   * If increment_ref is true (the default) the constructor will increment the
   * reference count on the libusb_device.
   *
   * If increment_ref is false the UsbDevice will skip incrementing the
   * reference count.  This can be useful if the caller has a reference that
   * they no longer need and want to transfer to the new UsbDevice.
   *
   * UsbDevice will always decrement the reference count when it is destroyed.
   */
  explicit UsbDevice(libusb_device* dev, bool increment_ref = true);
  ~UsbDevice();

  // Move operators
  UsbDevice(UsbDevice&& other) noexcept;
  UsbDevice& operator=(UsbDevice&& other) noexcept;

  void reset();

  UsbHandle open();

  /*
   * Find a device with a specific vendor and product ID.
   *
   * This returns the first device with the specified vendor and product ID.
   */
  static UsbDevice find(libusb_context* ctx, uint16_t vendor, uint16_t product);

 private:
  UsbDevice(const UsbDevice& other) = delete;
  UsbDevice& operator=(UsbDevice& other) = delete;

  libusb_device* dev_{nullptr};
};

} // namespace facebook::fboss
