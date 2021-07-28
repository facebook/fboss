// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/FpgaDevice.h"

namespace facebook::fboss {

/**
 * HwMemoryRegion models a contiguous memory region. IO can be any transport
 * class that supports read and write (for example FpgaDevice)
 */
template <typename IO>
class HwMemoryRegion {
 public:
  HwMemoryRegion(
      const std::string& name,
      IO* device,
      uint32_t start,
      uint32_t size)
      : name_(name), device_(device), start_(start), size_(size) {
    XLOG(DBG2) << folly::format(
        "Creating memory region {} at address={:#x} size={:d}",
        name_,
        start_,
        size_);
  }

  uint32_t read(uint32_t offset) const {
    XLOG(DBG5) << folly::format(
        "Request to read Memory region {} offset: {:#x}", name_, offset);
    XLOG(DBG5) << folly::format("start: {:#x} size_: {:#x}", start_, size_);
    CHECK(offset >= 0 && offset < size_ - 3);
    const uint32_t ret = device_->read(start_ + offset);
    XLOG(DBG5) << folly::format(
        "Memory region {} read from {:#x}={:#x}", name_, start_ + offset, ret);
    return ret;
  }

  void write(uint32_t offset, uint32_t value) {
    XLOG(DBG5) << folly::format(
        "Request to write Memory region {} offset: {:#x} value: {:#x}",
        name_,
        offset,
        value);
    XLOG(DBG5) << folly::format("start: {:#x} size_: {:#x}", start_, size_);
    CHECK(offset >= 0 && offset < size_ - 3);
    XLOG(DBG5) << folly::format(
        "Memory region {} write {:#x} to {:#x}", name_, value, offset);
    device_->write(start_ + offset, value);
    XLOG(DBG5) << folly::format(
        "Memory {} wrote to {:#x}={:#x}", name_, start_ + offset, value);
  }

  const std::string& getName() const {
    return name_;
  }

  IO* getDevice() const {
    return device_;
  }

  uint32_t getBaseAddress() {
    return start_;
  }

 private:
  const std::string name_;
  IO* device_;
  const uint32_t start_;
  const uint32_t size_;
};

using FpgaMemoryRegion = HwMemoryRegion<FpgaDevice>;
} // namespace facebook::fboss
