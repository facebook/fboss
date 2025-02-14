// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/weutil/FbossEepromParser.h"

namespace facebook::fboss::platform {

class CachedFbossEepromParser {
 public:
  CachedFbossEepromParser() {}

  std::vector<std::pair<std::string, std::string>> getContents(
      const std::string& eepromFilePath,
      uint16_t offset = 0) {
    auto eepromPtr = std::make_pair(eepromFilePath, offset);
    if (cache_.find(eepromPtr) == cache_.end()) {
      auto contents = FbossEepromParser(eepromFilePath, offset).getContents();
      cache_.insert({eepromPtr, contents});
      return contents;
    } else {
      return cache_.at(eepromPtr);
    }
  }

  std::optional<std::string> getProductName(
      const std::string& eepromFilePath,
      uint16_t offset = 0) {
    for (const auto& [key, value] : getContents(eepromFilePath, offset)) {
      if (key == "Product Name") {
        return value;
      }
    }
    return std::nullopt;
  }

  std::optional<int> getProductionState(
      const std::string& eepromFilePath,
      uint16_t offset = 0) {
    for (const auto& [key, value] : getContents(eepromFilePath, offset)) {
      // < V6 - "Product Production State"
      // = V6 - "Production State"
      if (key == "Product Production State" || key == "Production State") {
        return std::stoi(value);
      }
    }
    return std::nullopt;
  }

  std::optional<int> getProductionSubState(
      const std::string& eepromFilePath,
      uint16_t offset = 0) {
    for (const auto& [key, value] : getContents(eepromFilePath, offset)) {
      // < V6 - "Product Version"
      // = V6 - "Production Sub-State"
      if (key == "Product Version" || key == "Production Sub-State") {
        return std::stoi(value);
      }
    }
    return std::nullopt;
  }

  std::optional<int> getVariantVersion(
      const std::string& eepromFilePath,
      uint16_t offset = 0) {
    for (const auto& [key, value] : getContents(eepromFilePath, offset)) {
      // < V6 - "Product Sub-Version"
      // = V6 - "Re-Spin/Variant Indicator"
      if (key == "Product Sub-Version" || key == "Re-Spin/Variant Indicator") {
        return std::stoi(value);
      }
    }
    return std::nullopt;
  }

 private:
  std::map<
      std::pair<std::string /* eepromFilePath */, uint16_t /* offset */>,
      std::vector<std::pair<
          std::string /* eeprom key */,
          std::string /* eeprom value */>>>
      cache_{};
};

} // namespace facebook::fboss::platform
