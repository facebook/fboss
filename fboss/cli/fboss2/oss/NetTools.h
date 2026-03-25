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

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace nettools {
namespace bgplib {

// Stub for BGP origin attribute enum
enum class BgpAttrOrigin {
  IGP = 0,
  EGP = 1,
  INCOMPLETE = 2,
};

// Stub for BGP community attribute
class BgpAttrCommunityC {
 public:
  // Convert 32-bit community value to asn:value format
  explicit BgpAttrCommunityC(const std::string& commStr) : commStr_(commStr) {}

  static std::shared_ptr<BgpAttrCommunityC> createBgpAttrCommunity(
      const std::string& commStr) {
    return std::make_shared<BgpAttrCommunityC>(commStr);
  }

  std::string to_string() const {
    // Convert 32-bit community to asn:value format
    // BGP communities are encoded as: (asn << 16) | value
    try {
      uint32_t comm32 = std::stoul(commStr_);
      uint16_t asn = (comm32 >> 16) & 0xFFFF;
      uint16_t value = comm32 & 0xFFFF;
      return std::to_string(asn) + ":" + std::to_string(value);
    } catch (...) {
      // If conversion fails, return original string
      return commStr_;
    }
  }

 private:
  std::string commStr_;
};

// Well-known BGP communities (subset)
// Format: "asn:value" -> "ALIAS"
inline const std::map<std::string, std::string>& getWellKnownCommunities() {
  static const std::map<std::string, std::string> wellKnown = {
      // Meta/Facebook communities (common ones)
      {"65527:12705", "FABRIC_POD_RSW_LOOP"},
      {"65527:12706", "FABRIC_POD_FSW_LOOP"},
      {"65000:1", "NO_EXPORT"},
      {"65000:2", "NO_ADVERTISE"},
      {"65000:3", "NO_EXPORT_SUBCONFED"},
      // Add more as needed
  };
  return wellKnown;
}

// Stub for finding communities in a set
// Returns a map of community sets to their aliases
inline std::map<std::set<std::string>, std::string> findCommunities(
    const std::vector<std::string>& communities,
    const std::map<std::set<std::string>, std::string>& /* communitySet */) {
  // Look up well-known communities and return with aliases
  std::map<std::set<std::string>, std::string> result;
  const auto& wellKnown = getWellKnownCommunities();

  for (const auto& comm : communities) {
    std::set<std::string> commSet;
    commSet.insert(comm);

    // Check if this is a well-known community
    auto it = wellKnown.find(comm);
    if (it != wellKnown.end()) {
      result[commSet] = it->second; // Use the alias
    } else {
      result[commSet] = ""; // No alias - will show just the numeric value
    }
  }
  return result;
}

// Stub for BgpPathC topology info conversion
class BgpPathC {
 public:
  // Accept any type for topoInfoToStr - we don't know the exact type
  template <typename T>
  static std::string topoInfoToStr(const T& /* values */) {
    // Return placeholder - tests will show what's expected
    return "TopoInfo(stub)";
  }
};

// Constant for null/empty message
constexpr const char* kNullMessage = "";

// Stub for device type check
inline bool isAristaDevice() {
  // Return false - not an Arista device
  return false;
}

} // namespace bgplib
} // namespace nettools
