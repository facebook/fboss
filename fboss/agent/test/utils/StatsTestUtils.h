// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <sstream>
#include "fboss/agent/StatPrinters.h"
#include "fboss/agent/if/gen-cpp2/multiswitch_ctrl_types.h"

namespace facebook::fboss {
template <typename ValueT>
std::string _defaultDeltaValuePrinter(
    const ValueT& before,
    const ValueT& after) {
  std::stringstream ss;
  ss << " Before: " << before << std::endl << "After: " << after << std::endl;
  return ss.str();
}

template <typename MapTypeT>
std::string statsMapDelta(
    const MapTypeT& before,
    const MapTypeT& after,
    const std::function<std::string(
        const typename MapTypeT::mapped_type& before,
        const typename MapTypeT::mapped_type& after)>& deltaValuePrinter =
        _defaultDeltaValuePrinter<typename MapTypeT::mapped_type>) {
  std::stringstream ss;
  for (const auto& [key, beforeVal] : before) {
    auto aitr = after.find(key);
    if (aitr == after.end()) {
      ss << "Missing key in after: " << key << std::endl;
    } else if (beforeVal != aitr->second) {
      ss << " Stats did not match for : " << key
         << deltaValuePrinter(beforeVal, aitr->second);
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

std::string statsDelta(
    const multiswitch::HwSwitchStats& before,
    const multiswitch::HwSwitchStats& after);

std::string statsDelta(
    const std::map<uint16_t, multiswitch::HwSwitchStats>& before,
    const std::map<uint16_t, multiswitch::HwSwitchStats>& after);
} // namespace facebook::fboss
