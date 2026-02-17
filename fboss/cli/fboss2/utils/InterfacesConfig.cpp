/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/utils/InterfacesConfig.h"

#include <fmt/format.h>
#include <algorithm>
#include <unordered_set>

namespace facebook::fboss::utils {

namespace {
// Set of known attribute names (lowercase for case-insensitive comparison)
const std::unordered_set<std::string> kKnownAttributes = {
    "description",
    "mtu",
};
} // namespace

bool InterfacesConfig::isKnownAttribute(const std::string& s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  return kKnownAttributes.find(lower) != kKnownAttributes.end();
}

InterfacesConfig::InterfacesConfig(std::vector<std::string> v)
    : interfaces_(std::vector<std::string>{}) {
  if (v.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  // Find where port names end and attributes begin
  // Ports are all tokens before the first known attribute name
  size_t attrStart = v.size();
  for (size_t i = 0; i < v.size(); ++i) {
    if (isKnownAttribute(v[i])) {
      attrStart = i;
      break;
    }
  }

  // Must have at least one port name
  if (attrStart == 0) {
    throw std::invalid_argument(
        fmt::format(
            "No interface name provided. First token '{}' is an attribute name.",
            v[0]));
  }

  // Extract port names
  std::vector<std::string> portNames(v.begin(), v.begin() + attrStart);

  // Parse attribute-value pairs
  for (size_t i = attrStart; i < v.size(); i += 2) {
    const std::string& attr = v[i];

    if (!isKnownAttribute(attr)) {
      throw std::invalid_argument(
          fmt::format(
              "Unknown attribute '{}'. Valid attributes are: description, mtu",
              attr));
    }

    if (i + 1 >= v.size()) {
      throw std::invalid_argument(
          fmt::format("Missing value for attribute '{}'", attr));
    }

    const std::string& value = v[i + 1];

    // Check if "value" is actually another attribute name (user forgot value)
    if (isKnownAttribute(value)) {
      throw std::invalid_argument(
          fmt::format(
              "Missing value for attribute '{}'. Got another attribute '{}' instead.",
              attr,
              value));
    }

    // Normalize attribute name to lowercase
    std::string attrLower = attr;
    std::transform(
        attrLower.begin(), attrLower.end(), attrLower.begin(), ::tolower);
    attributes_.emplace_back(attrLower, value);
  }

  // Now resolve the port names to InterfaceList
  // This will throw if any port is not found
  interfaces_ = InterfaceList(std::move(portNames));
}

} // namespace facebook::fboss::utils
