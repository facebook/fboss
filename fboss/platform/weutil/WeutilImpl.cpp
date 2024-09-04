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

  int cursor = 0;
  std::vector<std::string> items;
  for (auto [key, value] : info) {
    // CRC16 is not needed in JSON output
    if (key == "CRC16") {
      continue;
    }
    items.push_back(fmt::format("\"{}\": \"{}\"", key, value));
  }
  int vectorSize = items.size();
  // Manually create JSON object without using folly, so that this code
  // will be ported to BMC later
  // Print the first part of the JSON - fixed entry to make a JSON
  std::cout << "{";
  std::cout << "\"Information\": {";
  // Print the second part of the JSON, the dynamic entry
  for (auto& item : items) {
    std::cout << item;
    if (cursor++ != vectorSize - 1) {
      std::cout << ", ";
    }
  }

  // Finally, print the third part of the JSON - fixed entry
  std::cout << "}, \"Actions\": [], \"Resources\": []";
  std::cout << "}" << std::endl;
}

} // namespace facebook::fboss::platform
