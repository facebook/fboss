/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/UsbHandle.h"

#include <folly/ScopeGuard.h>

#include <glog/logging.h>
#include <libusb-1.0/libusb.h>

#include "fboss/lib/usb/UsbError.h"

namespace facebook::fboss {
UsbHandle::UsbHandle(UsbHandle&& other) noexcept : handle_(other.handle_) {
  other.handle_ = nullptr;
}

UsbHandle& UsbHandle::operator=(UsbHandle&& other) noexcept {
  std::swap(handle_, other.handle_);
  other.close();
  return *this;
}

void UsbHandle::openFrom(libusb_device* dev, bool autoDetach) {
  close();
  int rc = libusb_open(dev, &handle_);
  if (rc != 0) {
    throw LibusbError(rc, "failed to open USB-to-I2C bridge");
  }
  SCOPE_FAIL {
    close();
  };
  if (autoDetach) {
    setAutoDetachKernelDriver(true);
  }
}

void UsbHandle::close() {
  if (!handle_) {
    return;
  }

  // The libusb docs claim we should call libusb_release_interface()
  // for each interface we claimed before closing.  I have a hard time
  // believing the OS wouldn't release this for us automatically though,
  // and I haven't seen any issues by not explicitly releasing interfaces
  // prior to close.
  //
  // For now we don't bother.  If this in fact turns out to cause issues,
  // we could keep a bitmap of claimed interfaces so we can release them
  // here.
  libusb_close(handle_);
  handle_ = nullptr;
}

void UsbHandle::setAutoDetachKernelDriver(bool autoDetach) {
  int value = autoDetach;
  auto rc = libusb_set_auto_detach_kernel_driver(handle_, value);
  if (rc != 0) {
    throw LibusbError(
        rc,
        "failed to set auto-detach driver for "
        "USB-to-I2C bridge");
  }
}

void UsbHandle::claimInterface(int iface) {
  DCHECK(isOpen());
  auto rc = libusb_claim_interface(handle_, iface);
  if (rc != 0) {
    throw LibusbError(rc, "failed to claim interface ", iface);
  }
}

} // namespace facebook::fboss
