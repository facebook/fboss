// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace facebook::fboss::platform {

using EepromContents = std::vector<
    std::pair<std::string /* eeprom key */, std::string /* eeprom value */>>;

class FbossEepromParserUtils {
 public:
  static std::optional<std::string> getProductName(
      const EepromContents& contents);
  static std::optional<int> getProductionState(const EepromContents& contents);
  static std::optional<int> getProductionSubState(
      const EepromContents& contents);
  static std::optional<int> getVariantVersion(const EepromContents& contents);
};

} // namespace facebook::fboss::platform
