// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <sstream>

namespace facebook::fboss {
template <typename MapTypeT>
std::string statsMapDelta(const MapTypeT& before, const MapTypeT& after) {
  std::stringstream ss;
  for (const auto& [key, beforeVal] : before) {
    auto aitr = after.find(key);
    if (aitr == after.end()) {
      ss << "Missing key in after: " << key << std::endl;
    } else if (beforeVal != aitr->second) {
      ss << " Stats did not match for : " << key << " Before: " << beforeVal
         << std::endl
         << " After: " << aitr->second << std::endl;
    }
  }
  for (const auto& [key, afterVal] : after) {
    auto bitr = before.find(key);
    if (bitr == before.end()) {
      ss << "Missing key in before: " << key << std::endl;
    }
  }
  return ss.str();
}
} // namespace facebook::fboss
