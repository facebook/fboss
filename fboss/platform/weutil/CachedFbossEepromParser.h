// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <utility>
#include "fboss/platform/weutil/FbossEepromParser.h"

namespace facebook::fboss::platform {

class CachedFbossEepromParser {
 public:
  CachedFbossEepromParser() {}

  FbossEepromInterface getContents(
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
    return contents.getProductName();
  }

  std::optional<int> getProductionState(
      const std::string& eepromFilePath,
      uint16_t offset) {
    auto contents = getContents(eepromFilePath, offset);
    return std::stoi(contents.getProductionState());
  }

  std::optional<int> getProductionSubState(
      const std::string& eepromFilePath,
      uint16_t offset) {
    auto contents = getContents(eepromFilePath, offset);
    return std::stoi(contents.getProductionSubState());
  }

  std::optional<int> getVariantVersion(
      const std::string& eepromFilePath,
      uint16_t offset = 0) {
    auto contents = getContents(eepromFilePath, offset);
    return stoi(contents.getVariantVersion());
  }

 private:
  std::map<
      std::pair<std::string /* eepromFilePath */, uint16_t /* offset */>,
      FbossEepromInterface>
      cache_{};
};

} // namespace facebook::fboss::platform
