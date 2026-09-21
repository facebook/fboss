// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include <fmt/format.h>
#include <re2/re2.h>

namespace facebook::fboss::platform::platform_manager {

inline const re2::RE2 kI2cAddrRe{"0x[0-9a-f]{2}"};

struct I2cAddr {
 public:
  explicit I2cAddr(uint16_t addr) : addr_(addr) {}
  explicit I2cAddr(const std::string& addr)
      : addr_(std::stoi(addr, nullptr, 16 /* base */)) {
    if (!re2::RE2::FullMatch(addr, kI2cAddrRe)) {
      throw std::invalid_argument("Invalid i2c addr: " + addr);
    }
  }
  bool operator==(const I2cAddr& b) const {
    return addr_ == b.addr_;
  }
  // Returns string in the format 0x0f
  std::string hex2Str() const {
    return fmt::format("{:#04x}", addr_);
  }
  // Returns string in the format 000f
  std::string hex4Str() const {
    return fmt::format("{:04x}", addr_);
  }
  // Returns integer
  uint16_t raw() const {
    return addr_;
  }

 private:
  uint16_t addr_{0};
};

} // namespace facebook::fboss::platform::platform_manager
