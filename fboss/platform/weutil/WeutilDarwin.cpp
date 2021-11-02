// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/WeutilDarwin.h"
#include <folly/Conv.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include "fboss/platform/helpers/Utils.h"
#include "fboss/platform/weutil/prefdl/prefdl.h"

using namespace facebook::fboss::platform::helpers;

namespace facebook::fboss::platform {

WeutilDarwin::WeutilDarwin() {
  std::string ret;
  int retVal = 0;

  if (!std::filesystem::exists(kPathPrefix_)) {
    if (!std::filesystem::create_directory(kPathPrefix_)) {
      throw std::runtime_error("Cannot create directory: " + kPathPrefix_);
    }
  }

  ret = execCommand(kCreateLayout_, retVal);

  if (retVal) {
    throw std::runtime_error(
        "Cannot create layout file with: " + kCreateLayout_);
  }

  ret = execCommand(kFlashRom_, retVal);

  if (retVal) {
    throw std::runtime_error(
        "Cannot create BIOS file with: " + kFlashRom_ + " " +
        std::to_string(retVal));
  }

  ret = execCommand(kddComands_, retVal);

  if (retVal) {
    throw std::runtime_error("Cannot create prefdl file with: " + kddComands_);
  }
}

void WeutilDarwin::printInfo() {
  PrefdlBase prefdl(kPredfl_);

  for (auto item : weFields_) {
    if (item.first == "Wedge EEPROM") {
      std::cout << item.first << folly::to<std::string>(" ", item.second, ":")
                << std::endl;
    } else {
      auto it = kMapping_.find(item.first);
      std::cout << folly::to<std::string>(item.first, ": ")
                << (it == kMapping_.end() ? item.second
                                          : prefdl.getField(it->second))
                << std::endl;
    }
  }
}

} // namespace facebook::fboss::platform
