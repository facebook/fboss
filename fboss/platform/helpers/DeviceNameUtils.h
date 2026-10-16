// Copyright (c) Meta Platforms, Inc. and affiliates.

#pragma once

#include <array>
#include <string>
#include <string_view>

namespace facebook::fboss::platform {

// Optional vendor specific prefix in front of a generic device name.
inline constexpr std::array<std::string_view, 1> kVendorPrefixes{
    "nh_", // Nexthop AI
};

// "^(<valid_vendor_pfx>)?<base>$", for names validated with a regex.
inline std::string vendorPrefixedPattern(std::string_view base) {
  std::string prefixes;
  for (auto prefix : kVendorPrefixes) {
    prefixes += prefixes.empty() ? "" : "|";
    prefixes += prefix;
  }
  return "^(" + prefixes + ")?" + std::string(base) + "$";
}

// True for deviceNameBase or a registered vendor prefix followed by it.
inline bool isDeviceName(
    std::string_view deviceName,
    std::string_view deviceNameBase) {
  if (deviceName == deviceNameBase) {
    return true;
  }
  for (auto prefix : kVendorPrefixes) {
    if (deviceName.size() == prefix.size() + deviceNameBase.size() &&
        deviceName.starts_with(prefix) &&
        deviceName.ends_with(deviceNameBase)) {
      return true;
    }
  }
  return false;
}

} // namespace facebook::fboss::platform
