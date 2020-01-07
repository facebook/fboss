/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/UsbDevice.h"

#include <folly/ScopeGuard.h>

#include <libusb-1.0/libusb.h>

#include "fboss/lib/usb/UsbError.h"
#include "fboss/lib/usb/UsbHandle.h"

namespace facebook::fboss {
UsbDevice::UsbDevice(libusb_device* dev, bool increment_ref) : dev_(dev) {
  if (increment_ref) {
    libusb_ref_device(dev_);
  }
}

UsbDevice::~UsbDevice() {
  reset();
}

UsbDevice::UsbDevice(UsbDevice&& other) noexcept : dev_(other.dev_) {
  other.dev_ = nullptr;
}

UsbDevice& UsbDevice::operator=(UsbDevice&& other) noexcept {
  std::swap(dev_, other.dev_);
  other.reset();
  return *this;
}

void UsbDevice::reset() {
  if (dev_) {
    libusb_unref_device(dev_);
    dev_ = nullptr;
  }
}

UsbHandle UsbDevice::open() {
  UsbHandle ret;
  ret.openFrom(dev_);
  return ret;
}

UsbDevice
UsbDevice::find(libusb_context* ctx, uint16_t vendor, uint16_t product) {
  libusb_device** devList;
  ssize_t numDevices = libusb_get_device_list(ctx, &devList);
  if (numDevices < 0) {
    throw LibusbError(numDevices, "failed to list USB devices");
  }
  SCOPE_EXIT {
    libusb_free_device_list(devList, true);
  };

  for (ssize_t n = 0; n < numDevices; ++n) {
    libusb_device* dev = devList[n];

    libusb_device_descriptor desc;
    int rc = libusb_get_device_descriptor(dev, &desc);
    if (rc != 0) {
      fprintf(
          stderr,
          "failed to query device descriptor for device %zd: %s\n",
          n,
          libusb_error_name(rc));
      continue;
    }

    if (desc.idVendor == vendor && desc.idProduct == product) {
      return UsbDevice(dev);
    }
  }

  throw UsbError("device not found");
}

} // namespace facebook::fboss
