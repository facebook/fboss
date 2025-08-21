// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/showtech/I2cHelper.h"
#include <fmt/core.h>
#include <folly/String.h>

#include <iostream>
#include <map>
#include <sstream>

namespace facebook::fboss::platform {

std::map<uint16_t, std::string> I2cHelper::findI2cBuses() {
  std::map<uint16_t /* busNum */, std::string /* busName */> busMap;
  auto [ret, output] = platformUtils_.execCommand("i2cdetect -l");
  if (ret != 0) {
    std::cerr << "Failed to run i2cdetect -l" << std::endl;
    return {};
  }

  std::istringstream outputStream(output);
  std::string line;
  while (std::getline(outputStream, line)) {
    std::vector<std::string> parts;
    folly::split('\t', line, parts);
    if (parts.size() < 4) {
      std::cerr << fmt::format(
                       "Invalid output line from i2cdetect -l. `{}`", line)
                << std::endl;
      continue;
    }
    uint16_t busNum = std::stoul(parts[0].substr(4));
    busMap[busNum] = folly::trimWhitespace(parts[2]).str();
  }
  return busMap;
}

} // namespace facebook::fboss::platform
