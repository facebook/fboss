// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/platform/weutil/FbossEepromParser.h"
#include "fboss/platform/weutil/FbossEepromParserUtils.h"

namespace facebook::fboss::platform {

class CachedFbossEepromParser {
 public:
  CachedFbossEepromParser() {}

  std::vector<std::pair<std::string, std::string>> getContents(
      const std::string& eepromFilePath,
      uint16_t offset) {
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
      uint16_t offset) {
    auto contents = getContents(eepromFilePath, offset);
    return FbossEepromParserUtils::getProductName(contents);
  }

  std::optional<int> getProductionState(
      const std::string& eepromFilePath,
      uint16_t offset) {
    auto contents = getContents(eepromFilePath, offset);
    return FbossEepromParserUtils::getProductionState(contents);
  }

  std::optional<int> getProductionSubState(
      const std::string& eepromFilePath,
      uint16_t offset) {
    auto contents = getContents(eepromFilePath, offset);
    return FbossEepromParserUtils::getProductionSubState(contents);
  }

  std::optional<int> getVariantVersion(
      const std::string& eepromFilePath,
      uint16_t offset = 0) {
    auto contents = getContents(eepromFilePath, offset);
    return FbossEepromParserUtils::getVariantVersion(contents);
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
