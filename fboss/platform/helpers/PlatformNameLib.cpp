// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/helpers/PlatformNameLib.h"

#include <fb303/ServiceData.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

namespace {

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

} // namespace

namespace facebook::fboss::platform::helpers {

std::string PlatformNameLib::getPlatformNameFromBios(
    const PlatformUtils& platformUtils) const {
  XLOG(INFO) << "Getting platform name from bios using dmedicode ...";
  auto [exitStatus, standardOut] = platformUtils.execCommand(dmidecodeCommand);
  if (exitStatus != 0) {
    XLOG(ERR) << "Failed to get platform name from bios: " << stdout;
    throw std::runtime_error("Failed to get platform name from bios");
  }
  standardOut = folly::trimWhitespace(standardOut).str();
  XLOG(INFO) << "Platform name inferred from bios: " << standardOut;
  return sanitizePlatformName(standardOut);
}

std::optional<std::string> PlatformNameLib::getPlatformName(
    const PlatformUtils& platformUtils) const {
  try {
    auto nameFromBios = getPlatformNameFromBios(platformUtils);
    fb303::fbData->incrementCounter(kPlatformNameBiosReads, 1);
    return nameFromBios;
  } catch (const std::exception& e) {
    fb303::fbData->incrementCounter(kPlatformNameBiosReadFailures, 1);
    return std::nullopt;
  }
}

} // namespace facebook::fboss::platform::helpers
