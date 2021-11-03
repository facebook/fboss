// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/WeutilDarwin.h"
#include <folly/Conv.h>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include "fboss/platform/helpers/Utils.h"
#include "fboss/platform/weutil/prefdl/Prefdl.h"

using namespace facebook::fboss::platform::helpers;

namespace {
const std::string kPathPrefix = "/tmp/WeutilDarwin/";
const std::string kPredfl = kPathPrefix + "system-prefdl-bin";
const std::string kCreteLayout =
    "echo \"00001000:0001efff prefdl\" > " + kPathPrefix + "layout";
const std::string kFlashRom = "flashrom -p internal -l " + kPathPrefix +
    "layout -i prefdl -r " + kPathPrefix + "/bios > /dev/null 2>&1";
const std::string kddComands = "dd if=" + kPathPrefix + "/bios of=" + kPredfl +
    " bs=1 skip=8192 count=61440 > /dev/null 2>&1";

// Map weutil fields to prefld fields
const std::unordered_map<std::string, std::string> kMapping{
    {"Product Name", "SID"},
    {"Product Part Number", "SKU"},
    {"System Assembly Part Number", "ASY"},
    {"ODM PCBA Part Number", "PCA"},
    {"Product Version", "HwVer"},
    {"Product Sub-Version", "HwSubVer"},
    {"System Manufacturing Date", "MfgTime2"},
    {"Local MAC", "MAC"},
    {"Product Serial Number", "SerialNumber"},
};

} // namespace
namespace facebook::fboss::platform {

WeutilDarwin::WeutilDarwin() {
  std::string ret;
  int retVal = 0;

  if (!std::filesystem::exists(kPathPrefix)) {
    if (!std::filesystem::create_directory(kPathPrefix)) {
      throw std::runtime_error("Cannot create directory: " + kPathPrefix);
    }
  }

  ret = execCommand(kCreteLayout, retVal);

  if (retVal) {
    throw std::runtime_error("Cannot create layout file with: " + kCreteLayout);
  }

  ret = execCommand(kFlashRom, retVal);

  if (retVal) {
    throw std::runtime_error(
        "Cannot create BIOS file with: " + kFlashRom + " " +
        std::to_string(retVal));
  }

  ret = execCommand(kddComands, retVal);

  if (retVal) {
    throw std::runtime_error("Cannot create prefdl file with: " + kddComands);
  }
}

void WeutilDarwin::printInfo() {
  PrefdlBase prefdl(kPredfl);

  for (auto item : weFields_) {
    if (item.first == "Wedge EEPROM") {
      std::cout << item.first << folly::to<std::string>(" ", item.second, ":")
                << std::endl;
    } else {
      auto it = kMapping.find(item.first);
      std::cout << folly::to<std::string>(item.first, ": ")
                << (it == kMapping.end() ? item.second
                                         : prefdl.getField(it->second))
                << std::endl;
    }
  }
}

} // namespace facebook::fboss::platform
