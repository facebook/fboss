// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/weutil/ContentValidator.h"

#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
std::string getProductionStateString(const std::string& value) {
  if (value == "1") {
    return "EVT";
  } else if (value == "2") {
    return "DVT";
  } else if (value == "3") {
    return "PVT";
  } else if (value == "4") {
    return "MP";
  }
  throw std::runtime_error(
      fmt::format("Invalid Production State with value: '{}'", value));
}
} // namespace

namespace facebook::fboss::platform {

WeutilImpl::WeutilImpl(const std::string& eepromPath, const uint16_t offset)
    : parser_(eepromPath, offset) {}

std::vector<std::pair<std::string, std::string>> WeutilImpl::getContents() {
  auto contents = parser_.getContents();
  if (!ContentValidator().isValid(contents)) {
    throw std::runtime_error("Invalid EEPROM contents");
  }
  return parser_.getContents();
}

void WeutilImpl::printInfo() {
  for (auto [key, value] : getContents()) {
    if (key == "Production State") {
      std::cout << key << ": " << getProductionStateString(value) << std::endl;
      continue;
    }
    std::cout << key << ": " << value << std::endl;
  }
}

void WeutilImpl::printInfoJson() {
  auto info = getContents();
  folly::dynamic eepromObject = folly::dynamic::object;
  for (const auto& [key, value] : parser_.getContents()) {
    if (key == "CRC16") {
      continue;
    }
    eepromObject[key] = value;
  }

  folly::dynamic json = folly::dynamic::object;
  json["Information"] = eepromObject;
  json["Actions"] = folly::dynamic::array();
  json["Resources"] = folly::dynamic::array();
  std::cout << folly::toPrettyJson(json) << std::endl;
}

} // namespace facebook::fboss::platform
