// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/FbossEepromParserUtils.h"

namespace facebook::fboss::platform {

std::optional<std::string> FbossEepromParserUtils::getProductName(
    const EepromContents& contents) {
  for (const auto& [key, value] : contents) {
    if (key == "Product Name") {
      return value;
    }
  }
  return std::nullopt;
}

std::optional<int> FbossEepromParserUtils::getProductionState(
    const EepromContents& contents) {
  for (const auto& [key, value] : contents) {
    // < V6 - "Product Production State"
    // = V6 - "Production State"
    if (key == "Product Production State" || key == "Production State") {
      return std::stoi(value);
    }
  }
  return std::nullopt;
}

std::optional<int> FbossEepromParserUtils::getProductionSubState(
    const EepromContents& contents) {
  for (const auto& [key, value] : contents) {
    // < V6 - "Product Version"
    // = V6 - "Production Sub-State"
    if (key == "Product Version" || key == "Production Sub-State") {
      return std::stoi(value);
    }
  }
  return std::nullopt;
}

std::optional<int> FbossEepromParserUtils::getVariantVersion(
    const EepromContents& contents) {
  for (const auto& [key, value] : contents) {
    // < V6 - "Product Sub-Version"
    // = V6 - "Re-Spin/Variant Indicator"
    if (key == "Product Sub-Version" || key == "Re-Spin/Variant Indicator") {
      return std::stoi(value);
    }
  }
  return std::nullopt;
}

} // namespace facebook::fboss::platform
