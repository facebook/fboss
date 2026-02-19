// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once

#include <optional>
#include <set>

namespace rackmon {
using UniqueDeviceAddress = std::pair<std::optional<uint8_t>, uint8_t>;

class DeviceLocationFilter {
 private:
  std::set<UniqueDeviceAddress> addrFilter_{};

 public:
  DeviceLocationFilter() = delete;
  explicit DeviceLocationFilter(const std::set<UniqueDeviceAddress>& addrFilter)
      : addrFilter_(addrFilter) {}
  explicit DeviceLocationFilter(const std::set<uint16_t>& combinedFilter) {
    for (auto c : combinedFilter) {
      addrFilter_.insert(decompose(c));
    }
  }

  static UniqueDeviceAddress decompose(uint16_t combined) {
    uint8_t devAddress = combined & 0xFF;
    uint8_t port = combined >> 8;
    if (port == 0) {
      return std::make_pair(std::nullopt, devAddress);
    }
    return std::make_pair(port, devAddress);
  }

  bool contains(std::optional<uint8_t> port, uint8_t addr) const {
    for (const auto& [p, a] : addrFilter_) {
      if (a == addr && (!p.has_value() || p == port)) {
        // Addresses match and either the ports match or
        // (for backwards compatibility) no port is specified
        return true;
      }
    }
    return false;
  }

  static uint16_t combine(std::optional<uint8_t> port, uint8_t devAddr) {
    uint16_t uniqueAddress = 0;
    if (port.has_value()) {
      uniqueAddress = port.value();
      uniqueAddress = uniqueAddress << 8;
    }
    uniqueAddress |= devAddr;
    return uniqueAddress;
  }
};

} // namespace rackmon
