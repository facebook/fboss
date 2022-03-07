// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/WeutilDarwin.h"
#include <fboss/platform/helpers/Utils.h>
#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json.h>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <utility>
#include "fboss/platform/helpers/Utils.h"
#include "fboss/platform/weutil/prefdl/Prefdl.h"

using namespace facebook::fboss::platform::helpers;

namespace {
const std::string kPathPrefix = "/tmp/WeutilDarwin";
const std::string kPredfl = kPathPrefix + "/system-prefdl-bin";
const std::string kCreteLayout =
    "echo \"00001000:0001efff prefdl\" > " + kPathPrefix + "/layout";
const std::string kFlashromGetFlashType = "flashrom -p internal ";

const std::string kFlashromGetContent = " -l " + kPathPrefix +
    "/layout -i prefdl -r " + kPathPrefix + "/bios > /dev/null 2>&1";

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
  int retVal = 0;
  std::string ret;

  if (!std::filesystem::exists(kPathPrefix)) {
    if (!std::filesystem::create_directory(kPathPrefix)) {
      throw std::runtime_error("Cannot create directory: " + kPathPrefix);
    }
  }

  ret = execCommandUnchecked(kCreteLayout, retVal);

  if (retVal != 0) {
    throw std::runtime_error("Cannot create layout file with: " + kCreteLayout);
  }

  // Get flash type
  ret = execCommandUnchecked(kFlashromGetFlashType + " 2>&1 ", retVal);

  /* Since flashrom will return 1 for "flashrom -p internal"
   * we ignore retVal == 1
   */

  if ((retVal != 0) && (retVal != 1)) {
    throw std::runtime_error(
        "Cannot get flash type with: " + kFlashromGetFlashType);
  }

  std::string getPrefdl;
  std::string flashType = getFlashType(ret);

  if (!flashType.empty()) {
    getPrefdl = folly::to<std::string>(
        kFlashromGetFlashType, " -c ", flashType, kFlashromGetContent);
  } else {
    getPrefdl =
        folly::to<std::string>(kFlashromGetFlashType, kFlashromGetContent);
  }

  ret = execCommandUnchecked(getPrefdl, retVal);

  if (retVal != 0) {
    throw std::runtime_error(folly::to<std::string>(
        "Cannot create BIOS file with: ",
        getPrefdl,
        " ",
        ", return value: ",
        std::to_string(retVal)));
  }

  ret = execCommandUnchecked(kddComands, retVal);

  if (retVal != 0) {
    throw std::runtime_error("Cannot create prefdl file with: " + kddComands);
  }
}

std::vector<std::pair<std::string, std::string>> WeutilDarwin::getInfo() {
  PrefdlBase prefdl(kPredfl);

  std::vector<std::pair<std::string, std::string>> ret;

  for (auto item : weFields_) {
    auto it = kMapping.find(item.first);
    ret.emplace_back(
        item.first,
        it == kMapping.end() ? item.second : prefdl.getField(it->second));
  }

  return ret;
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

void WeutilDarwin::printInfoJson() {
  PrefdlBase prefdl(kPredfl);
  folly::dynamic wedgeInfo = folly::dynamic::object;

  wedgeInfo["Actions"] = folly::dynamic::array();
  wedgeInfo["Resources"] = folly::dynamic::array();
  wedgeInfo["Information"] = folly::dynamic::object;

  for (auto item : weFields_) {
    if (item.first != "Wedge EEPROM") {
      auto it = kMapping.find(item.first);
      wedgeInfo["Information"][item.first] =
          (it == kMapping.end() ? item.second : prefdl.getField(it->second));
    }
  }

  // For Darwin BMC-less, we need to fill empty field "Extended MAC Address
  // Size" and "Extended MAC Base" (same as "Local MAC") as a workaround for
  // creating /var/facebook/fboss/fruid.json (T110038028)
  if (wedgeInfo["Information"]["Extended MAC Base"] == "") {
    wedgeInfo["Information"]["Extended MAC Base"] =
        wedgeInfo["Information"]["Local MAC"];
    wedgeInfo["Information"]["Extended MAC Address Size"] = "1";
  }
  std::cout << folly::toPrettyJson(wedgeInfo);
}

} // namespace facebook::fboss::platform
