// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <iostream>
#include <unordered_map>
#include <utility>

#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json.h>
#include <filesystem>

#include "fboss/platform/helpers/PlatformUtils.h"
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
const std::unordered_map<std::string, std::string> eepromDevMapping_{
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

// weutil output fields and default value for all FBOSS switches w/wo OpenBMC
const std::vector<std::pair<std::string, std::string>> weFields_{
    {"Wedge EEPROM", "CHASSIS"},
    {"Version", "0"},
    {"Product Name", ""},
    {"Product Part Number", ""},
    {"System Assembly Part Number", ""},
    {"Facebook PCBA Part Number", ""},
    {"Facebook PCB Part Number", ""},
    {"ODM PCBA Part Number", ""},
    {"ODM PCBA Serial Number", ""},
    {"Product Production State", "0"},
    {"Product Version", ""},
    {"Product Sub-Version", ""},
    {"Product Serial Number", ""},
    {"Product Asset Tag", ""},
    {"System Manufacturer", ""},
    {"System Manufacturing Date", ""},
    {"PCB Manufacturer", ""},
    {"Assembled At", ""},
    {"Local MAC", ""},
    {"Extended MAC Base", ""},
    {"Extended MAC Address Size", ""},
    {"Location on Fabric", ""},
    {"CRC8", "0x0"},
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
WeutilDarwin::WeutilDarwin(const std::string& eeprom) {
  eepromParser_ = std::make_unique<PrefdlBase>(getEepromPathFromName(eeprom));
}

std::string WeutilDarwin::getEepromPathFromName(const std::string& eeprom) {
  std::string eepromName(eeprom);
  std::transform(
      eepromName.begin(), eepromName.end(), eepromName.begin(), ::tolower);
  if (eepromName == "" || eepromName == "chassis") {
    genSpiPrefdlFile();
    return kPredfl;
  } else {
    auto itr = eepromDevMapping_.find(eepromName);
    if (itr != eepromDevMapping_.end() &&
        std::filesystem::exists(itr->second)) {
      return itr->second;
    } else {
      throw std::runtime_error("Invalid eeprom name: " + eepromName);
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

  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(kCreteLayout);
  if (exitStatus != 0) {
    throw std::runtime_error("Cannot create layout file with: " + kCreteLayout);
  }

  // Get flash type
  std::tie(exitStatus, standardOut) =
      PlatformUtils().execCommand(kFlashromGetFlashType + " 2>&1 ");

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

  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(getPrefdl);
  if (exitStatus != 0) {
    throw std::runtime_error(folly::to<std::string>(
        "Cannot create BIOS file with: ",
        getPrefdl,
        " ",
        ", return value: ",
        std::to_string(exitStatus)));
  }

  std::tie(exitStatus, standardOut) = PlatformUtils().execCommand(kddComands);
  if (exitStatus != 0) {
    throw std::runtime_error("Cannot create prefdl file with: " + kddComands);
  }
}

std::vector<std::pair<std::string, std::string>> WeutilDarwin::getInfo() {
  std::vector<std::pair<std::string, std::string>> ret;
  for (auto& item : weFields_) {
    auto it = kMapping.find(item.first);
    ret.emplace_back(
        item.first,
        it == kMapping.end() ? item.second
                             : eepromParser_->getField(it->second));
  }
  return ret;
}

void WeutilDarwin::printInfo() {
  for (auto item : weFields_) {
    if (item.first == "Wedge EEPROM") {
      std::cout << item.first << folly::to<std::string>(" ", item.second, ":")
                << std::endl;
    } else {
      auto it = kMapping.find(item.first);
      std::cout << folly::to<std::string>(item.first, ": ")
                << (it == kMapping.end() ? item.second
                                         : eepromParser_->getField(it->second))
                << std::endl;
    }
  }
}

void WeutilDarwin::printInfoJson() {
  folly::dynamic wedgeInfo = folly::dynamic::object;

  wedgeInfo["Actions"] = folly::dynamic::array();
  wedgeInfo["Resources"] = folly::dynamic::array();
  wedgeInfo["Information"] = folly::dynamic::object;

  for (auto item : weFields_) {
    if (item.first != "Wedge EEPROM") {
      auto it = kMapping.find(item.first);
      wedgeInfo["Information"][item.first] =
          (it == kMapping.end() ? item.second
                                : eepromParser_->getField(it->second));
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

void WeutilDarwin::printUsage(void) {
  std::cout
      << "weutil [--h] [--json] [--eeprom pem|fanspinner|rackmon|chassis(default)]"
      << std::endl;

  std::cout << "usage examples:" << std::endl;
  std::cout << "    weutil" << std::endl;
  std::cout << "    weutil --eeprom pem" << std::endl;
}

} // namespace facebook::fboss::platform
