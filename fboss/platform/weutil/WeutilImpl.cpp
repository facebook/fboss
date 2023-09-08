// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <unordered_map>
#include <utility>

#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <filesystem>
#include <string>

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/weutil/prefdl/Prefdl.h"

namespace facebook::fboss::platform {

WeutilImpl::WeutilImpl(const std::string& eeprom, PlainWeutilConfig config)
    : config_(config) {
  XLOG(INFO) << "WeutilImpl: eeprom = " << eeprom;
  // Store the eeprom name here, so that it will be
  // checked and the translated to the actual path in the logic later
  eepromPath = eeprom;
}

std::vector<std::pair<std::string, std::string>> WeutilImpl::getInfo(
    const std::string& eeprom) {
  std::vector<std::pair<std::string, std::string>> info;

  XLOG(INFO) << "getInfo: eeprom = " << eeprom;
  return info;
}

void WeutilImpl::printInfo() {
  XLOG(INFO) << "printInfo";
}

void WeutilImpl::printInfoJson() {
  XLOG(INFO) << "printInfoJson";
}

bool WeutilImpl::verifyOptions() {
  return true;
}
void WeutilImpl::printUsage() {
  XLOG(INFO) << "printUSage";
}

} // namespace facebook::fboss::platform
