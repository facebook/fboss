// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/showtech/I2cHelper.h"
#include <fmt/core.h>
#include <folly/String.h>

#include <map>
#include <sstream>

namespace facebook::fboss::platform {

std::map<uint16_t, std::string> I2cHelper::findI2cBuses() {
  std::map<uint16_t, std::string> busMap;
  auto [ret, output] = platformUtils_.execCommand("i2cdetect -l");
  if (ret) {
    return busMap;
  }

  std::istringstream outputStream(output);
  std::string line;
  while (std::getline(outputStream, line)) {
    std::vector<std::string> parts;
    std::string part;
    std::istringstream lineStream(line);
    while (std::getline(lineStream, part, '\t')) {
      parts.push_back(folly::trimWhitespace(part).str());
    }
    std::string adapterName = parts[2];
    uint16_t busNum = std::stoul(parts[0].substr(4));
    busMap[busNum] = adapterName;
  }
  return busMap;
}

} // namespace facebook::fboss::platform
