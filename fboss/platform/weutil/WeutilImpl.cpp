// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/WeutilImpl.h"

#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace facebook::fboss::platform {

WeutilImpl::WeutilImpl(const std::string& eepromPath, const uint16_t offset)
    : parser_(eepromPath, offset) {}

std::vector<std::pair<std::string, std::string>> WeutilImpl::getContents() {
  return parser_.getContents();
}

void WeutilImpl::printInfo() {
  for (const auto& item : getContents()) {
    std::cout << item.first << ": " << item.second << std::endl;
  }
  return;
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
