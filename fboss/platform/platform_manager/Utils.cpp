// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/Utils.h"

#include <cctype>
#include <filesystem>
#include <stdexcept>

#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/platform_manager/ConfigValidator.h"

namespace fs = std::filesystem;
using namespace facebook::fboss::platform;

namespace {
const re2::RE2 kPmDeviceParseRe{"(?P<SlotPath>.*)\\[(?P<DeviceName>.*)\\]"};

std::string getPlatformNameFromBios() {
  XLOG(INFO) << "Getting platform name from bios using dmedicode ...";
  auto [exitStatus, standardOut] =
      PlatformUtils().execCommand("dmidecode -s system-product-name");
  if (exitStatus != 0) {
    XLOG(ERR) << "Failed to get platform name from bios: " << stdout;
    throw std::runtime_error("Failed to get platform name from bios");
  }
  standardOut = folly::trimWhitespace(standardOut).str();
  XLOG(INFO) << "Platform name inferred from bios: " << standardOut;
  return standardOut;
}
} // namespace

namespace facebook::fboss::platform::platform_manager {

// Some platforms do not have the standardized platform-names in dmidecode yet.
// For such platforms, we use a translation function to get the standardized
// platform-names.
std::string sanitizePlatformName(const std::string& platformNameFromBios) {
  std::string platformNameUpper(platformNameFromBios);
  std::transform(
      platformNameUpper.begin(),
      platformNameUpper.end(),
      platformNameUpper.begin(),
      ::toupper);

  if (platformNameUpper == "MINIPACK3" ||
      platformNameUpper == "MINIPACK3_MCB") {
    return "MONTBLANC";
  }

  if (platformNameUpper == "JANGA") {
    return "JANGA800BIC";
  }

  if (platformNameUpper == "TAHAN") {
    return "TAHAN800BC";
  }

  return platformNameUpper;
}

// Verify that the platform name from the config and dmidecode match.  This
// is necessary to prevent an incorrect config from being used on any platform.
void verifyPlatformNameMatches(
    const std::string& platformNameInConfig,
    const std::string& platformNameFromBios) {
  std::string platformNameInConfigUpper(platformNameInConfig);
  std::transform(
      platformNameInConfigUpper.begin(),
      platformNameInConfigUpper.end(),
      platformNameInConfigUpper.begin(),
      ::toupper);

  if (platformNameInConfigUpper == platformNameFromBios) {
    return;
  }

#ifndef IS_OSS
  if (platformNameFromBios == "NOT SPECIFIED") {
    XLOG(ERR)
        << "Platform name is not specified in BIOS. Skipping comparison with config.";
    return;
  }
#endif

  XLOGF(
      FATAL,
      "Platform name in config does not match the inferred platform name from "
      "bios. Config: {}, Inferred name from BIOS {} ",
      platformNameInConfigUpper,
      platformNameFromBios);
}

PlatformConfig Utils::getConfig() {
  std::string platformNameFromBios =
      sanitizePlatformName(getPlatformNameFromBios());
  std::string configJson =
      ConfigLib().getPlatformManagerConfig(platformNameFromBios);
  PlatformConfig config;
  try {
    apache::thrift::SimpleJSONSerializer::deserialize<PlatformConfig>(
        configJson, config);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to deserialize platform config: " << e.what();
    throw;
  }
  XLOG(DBG2) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      config);

  verifyPlatformNameMatches(*config.platformName(), platformNameFromBios);

  if (!ConfigValidator().isValid(config)) {
    XLOG(ERR) << "Invalid platform config";
    throw std::runtime_error("Invalid platform config");
  }
  return config;
}

bool Utils::createDirectories(const std::string& path) {
  std::error_code errCode;
  std::filesystem::create_directories(fs::path(path), errCode);
  if (errCode.value() != 0) {
    XLOG(ERR) << fmt::format(
        "Received error code {} from creating path {}", errCode.value(), path);
  }
  return errCode.value() == 0;
}

std::pair<std::string, std::string> Utils::parseDevicePath(
    const std::string& devicePath) {
  if (!ConfigValidator().isValidDevicePath(devicePath)) {
    throw std::runtime_error(fmt::format("Invalid DevicePath {}", devicePath));
  }
  std::string slotPath, deviceName;
  CHECK(RE2::FullMatch(devicePath, kPmDeviceParseRe, &slotPath, &deviceName));
  // Remove trailling '/' (e.g /abc/dfg/)
  CHECK_EQ(slotPath.back(), '/');
  if (slotPath.length() > 1) {
    slotPath.pop_back();
  }
  return {slotPath, deviceName};
}

std::string Utils::createDevicePath(
    const std::string& slotPath,
    const std::string& deviceName) {
  return fmt::format("{}/[{}]", slotPath == "/" ? "" : slotPath, deviceName);
}

} // namespace facebook::fboss::platform::platform_manager
