// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <iostream>
#include <unordered_map>
#include <utility>

#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json.h>
#include <filesystem>

#include "fboss/platform/helpers/Utils.h"
#include "fboss/platform/weutil/WeutilDarwin.h"
#include "fboss/platform/weutil/prefdl/Prefdl.h"

using namespace facebook::fboss::platform;

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

// Map eeprom_dev_type to symlink of sysfs path
const std::unordered_map<std::string, std::string> eeprom_dev_mapping{
    {"pem", "/run/devmap/eeproms/PEM_EEPROM"},
    {"fanspinner", "/run/devmap/eeproms/FANSPINNER_EEPROM"},
    {"rackmon", "/run/devmap/eeproms/RACKMON_EEPROM"},
    {"chassis", "/tmp/WeutilDarwin/system-prefdl-bin"},
};

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

const std::unordered_set<std::string> kFlashType = {
    "MX25L12805D",
    "N25Q128..3E"};

std::string getFlashType(const std::string& str) {
  for (auto& it : kFlashType) {
    if (str.find(it) != std::string::npos) {
      return it;
    }
  }
  return "";
}

} // namespace

namespace facebook::fboss::platform {
WeutilDarwin::WeutilDarwin(const std::string& eeprom) : eeprom_(eeprom) {
  std::transform(eeprom_.begin(), eeprom_.end(), eeprom_.begin(), ::tolower);
  if (eeprom_ == "" || eeprom_ == "chassis") {
    genSpiPrefdlFile();
  } else {
    // the symblink file should be created by udev rule already.
    auto _it = eeprom_dev_mapping.find(eeprom_);
    if (_it != eeprom_dev_mapping.end()) {
      if (!std::filesystem::exists(_it->second)) {
        throw std::runtime_error(
            "eeprom device: " + _it->second + " does not exist!");
      }
    }
  }
}

void WeutilDarwin::genSpiPrefdlFile(void) {
  int exitStatus = 0;
  std::string standardOut;

  if (!std::filesystem::exists(kPathPrefix)) {
    if (!std::filesystem::create_directory(kPathPrefix)) {
      throw std::runtime_error("Cannot create directory: " + kPathPrefix);
    }
  }

  std::tie(exitStatus, standardOut) = helpers::execCommand(kCreteLayout);
  if (exitStatus != 0) {
    throw std::runtime_error("Cannot create layout file with: " + kCreteLayout);
  }

  // Get flash type
  std::tie(exitStatus, standardOut) =
      helpers::execCommand(kFlashromGetFlashType + " 2>&1 ");

  /* Since flashrom will return 1 for "flashrom -p internal"
   * we ignore retVal == 1
   */

  if ((exitStatus != 0) && (exitStatus != 1)) {
    throw std::runtime_error(
        "Cannot get flash type with: " + kFlashromGetFlashType);
  }

  std::string getPrefdl;
  std::string flashType = getFlashType(standardOut);

  if (!flashType.empty()) {
    getPrefdl = folly::to<std::string>(
        kFlashromGetFlashType, " -c ", flashType, kFlashromGetContent);
  } else {
    getPrefdl =
        folly::to<std::string>(kFlashromGetFlashType, kFlashromGetContent);
  }

  std::tie(exitStatus, standardOut) = helpers::execCommand(getPrefdl);
  if (exitStatus != 0) {
    throw std::runtime_error(folly::to<std::string>(
        "Cannot create BIOS file with: ",
        getPrefdl,
        " ",
        ", return value: ",
        std::to_string(exitStatus)));
  }

  std::tie(exitStatus, standardOut) = helpers::execCommand(kddComands);
  if (exitStatus != 0) {
    throw std::runtime_error("Cannot create prefdl file with: " + kddComands);
  }
}

std::vector<std::pair<std::string, std::string>> WeutilDarwin::getInfo(
    const std::string&) {
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
  std::unique_ptr<PrefdlBase> pPrefdl;

  if (eeprom_ != "") {
    auto _it = eeprom_dev_mapping.find(eeprom_);
    if (_it == eeprom_dev_mapping.end()) {
      throw std::runtime_error("invalid eeprom type: " + eeprom_);
    }
    pPrefdl = std::make_unique<PrefdlBase>(_it->second);
  } else {
    pPrefdl = std::make_unique<PrefdlBase>(kPredfl);
  }

  for (auto item : weFields_) {
    if (item.first == "Wedge EEPROM") {
      std::cout << item.first << folly::to<std::string>(" ", item.second, ":")
                << std::endl;
    } else {
      auto it = kMapping.find(item.first);
      std::cout << folly::to<std::string>(item.first, ": ")
                << (it == kMapping.end() ? item.second
                                         : pPrefdl->getField(it->second))
                << std::endl;
    }
  }
}

void WeutilDarwin::printInfoJson() {
  std::unique_ptr<PrefdlBase> pPrefdl;
  folly::dynamic wedgeInfo = folly::dynamic::object;

  if (eeprom_ != "") {
    auto _it = eeprom_dev_mapping.find(eeprom_);
    if (_it == eeprom_dev_mapping.end()) {
      throw std::runtime_error("invalid eeprom type: " + eeprom_);
    }
    pPrefdl = std::make_unique<PrefdlBase>(_it->second);

  } else {
    pPrefdl = std::make_unique<PrefdlBase>(kPredfl);
  }

  wedgeInfo["Actions"] = folly::dynamic::array();
  wedgeInfo["Resources"] = folly::dynamic::array();
  wedgeInfo["Information"] = folly::dynamic::object;

  for (auto item : weFields_) {
    if (item.first != "Wedge EEPROM") {
      auto it = kMapping.find(item.first);
      wedgeInfo["Information"][item.first] =
          (it == kMapping.end() ? item.second : pPrefdl->getField(it->second));
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

bool WeutilDarwin::verifyOptions(void) {
  if (eeprom_ != "") {
    if (eeprom_ != "pem" && eeprom_ != "fanspinner" && eeprom_ != "rackmon" &&
        eeprom_ != "chassis") {
      printUsage();
      return false;
    }
  }
  return true;
}

void WeutilDarwin::printUsage(void) {
  std::cout
      << "weutil [--h] [--json] [--eeprom pem|fanspinner|rackmon|chassis(default)]"
      << std::endl;

  std::cout << "usage examples:" << std::endl;
  std::cout << "    weutil" << std::endl;
  std::cout << "    weutil --eeprom pem" << std::endl;
}

} // namespace facebook::fboss::platform
