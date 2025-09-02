// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/showtech/I2cHelper.h"
#include <fmt/core.h>
#include <folly/String.h>

#include <boost/bimap.hpp>
#include <iostream>
#include <sstream>

using I2cBusMap = boost::bimap<uint16_t, std::string>;

namespace facebook::fboss::platform {

I2cHelper::I2cHelper() {
  findI2cBuses();
}

void I2cHelper::findI2cBuses() {
  busMap_.clear();
  auto [ret, output] = platformUtils_.execCommand("i2cdetect -l");
  if (ret != 0) {
    std::cerr << "Failed to run i2cdetect -l" << std::endl;
    return;
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
    busMap_.insert({busNum, folly::trimWhitespace(parts[2]).str()});
  }
}

const I2cBusMap& I2cHelper::getI2cBusMap() const {
  return busMap_;
}

int I2cHelper::findI2cBusForScdI2cDevice(
    std::string scdPciAddr,
    uint16_t master,
    uint16_t bus) {
  auto adapter =
      fmt::format("SCD {} SMBus master {} bus {}", scdPciAddr, master, bus);
  auto it_right = busMap_.right.find(adapter);
  if (it_right != busMap_.right.end()) {
    return it_right->second;
  }
  return -1;
}

} // namespace facebook::fboss::platform
