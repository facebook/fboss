// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/ContentValidator.h"

#include <algorithm>
#include <iostream>
#include <map>

#include <fmt/format.h>

namespace facebook::fboss::platform {

bool ContentValidator::isValid(
    const std::vector<std::pair<std::string, std::string>>& contents) {
  std::map<std::string, std::string> contentsMap{};
  for (const auto& [key, value] : contents) {
    contentsMap[key] = value;
  }
  if (contentsMap.size() != contents.size()) {
    std::cerr << "Duplicate keys in EEPROM";
    return false;
  }

  // Validate Version
  const auto& versionIt = contentsMap.find("Version");
  if (versionIt == contentsMap.end()) {
    std::cerr << "Version not found in EEPROM";
    return false;
  }
  std::string version = versionIt->second;
  std::vector<std::string> validVersions = {"4", "5", "6"};
  if (std::find(validVersions.begin(), validVersions.end(), version) ==
      validVersions.end()) {
    std::cerr << fmt::format("Invalid Version: `{}`", version);
    return false;
  }

  // Validate Production State
  if (version == "6") {
    const auto& prodStateIt = contentsMap.find("Production State");
    if (prodStateIt == contentsMap.end()) {
      std::cerr << "Production State not found in EEPROM";
      return false;
    }
    std::string productionState = prodStateIt->second;
    auto validProductionStates = std::vector<std::string>{"1", "2", "3", "4"};
    if (std::find(
            validProductionStates.begin(),
            validProductionStates.end(),
            productionState) == validProductionStates.end()) {
      std::cerr << fmt::format(
          "Invalid Production State with value: '{}'", productionState);
      return false;
    }
  }
  return true;
};

} // namespace facebook::fboss::platform
