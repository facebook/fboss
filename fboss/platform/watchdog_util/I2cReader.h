// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace facebook::fboss::platform::watchdog_util {

class I2cReader {
 public:
  // Reads a single byte register via `i2cget -y -f <bus> 0x<addr> <reg>`.
  static std::optional<uint8_t>
  read(int bus, int addr, uint8_t reg, std::string& err);
};

} // namespace facebook::fboss::platform::watchdog_util
