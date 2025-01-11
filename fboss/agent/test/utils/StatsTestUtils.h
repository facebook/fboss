// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <sstream>
#include "fboss/agent/StatPrinters.h"
#include "fboss/agent/if/gen-cpp2/multiswitch_ctrl_types.h"

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

std::string statsMapDelta(
    const multiswitch::HwSwitchStats& before,
    const multiswitch::HwSwitchStats& after);
} // namespace facebook::fboss
