// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/helpers/PlatformUtils.h"

#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace facebook::fboss::platform {

WeutilImpl::WeutilImpl(const std::string& eepromPath, const uint16_t offset)
    : parser_(eepromPath, offset) {}

std::vector<std::pair<std::string, std::string>> WeutilImpl::getContents() {
  return parser_.getContents();
}

void WeutilImpl::printInfo() {
  for (const auto& item : parser_.getContents()) {
    std::cout << item.first << ": " << item.second << std::endl;
  }
  return;
}

void WeutilImpl::printInfoJson() {
  auto info = parser_.getContents();
  int vectorSize = info.size();
  int cursor = 0;
  // Manually create JSON object without using folly, so that this code
  // will be ported to BMC later
  // Print the first part of the JSON - fixed entry to make a JSON
  std::cout << "{";
  std::cout << "\"Information\": {";
  // Print the second part of the JSON, the dynamic entry
  for (auto [key, value] : info) {
    // CRC16 is not needed in JSON output
    if (key == "CRC16") {
      continue;
    }
    std::cout << "\"" << key << "\": ";
    std::cout << "\"" << value << "\"";
    if (cursor++ != vectorSize - 1) {
      std::cout << ", ";
    }
  }

  // Finally, print the third part of the JSON - fixed entry
  std::cout << "}, \"Actions\": [], \"Resources\": []";
  std::cout << "}" << std::endl;
}

} // namespace facebook::fboss::platform
