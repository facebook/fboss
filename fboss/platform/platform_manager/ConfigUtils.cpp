// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ConfigUtils.h"

#include <cctype>

#include <fmt/format.h>
#include <folly/String.h>
#include <folly/hash/Hash.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/platform_manager/ConfigValidator.h"

namespace facebook::fboss::platform::platform_manager {

using apache::thrift::SimpleJSONSerializer;

namespace {
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

  throw std::runtime_error(
      fmt::format(
          "Platform name in config does not match the inferred platform name from "
          "bios. Config: {}, Inferred name from BIOS {}",
          platformNameInConfigUpper,
          platformNameFromBios));
}

std::string computeHash(const PlatformConfig& config) {
  auto configJson = SimpleJSONSerializer::serialize<std::string>(config);
  auto hash = folly::hasher<std::string>{}(configJson);
  return fmt::format("{:016x}", hash);
}

} // namespace

ConfigUtils::ConfigUtils(
    std::shared_ptr<ConfigLib> configLib,
    std::shared_ptr<helpers::PlatformNameLib> platformNameLib,
    std::shared_ptr<PlatformFsUtils> platformFsUtils)
    : configLib_(std::move(configLib)),
      platformNameLib_(std::move(platformNameLib)),
      pFsUtils_(std::move(platformFsUtils)) {}

PlatformConfig ConfigUtils::getConfig() {
  if (config_.has_value()) {
    return config_.value();
  }
  std::string platformNameFromBios =
      platformNameLib_->getPlatformNameFromBios(true);
  std::string configJson =
      configLib_->getPlatformManagerConfig(platformNameFromBios);
  PlatformConfig config;
  try {
    SimpleJSONSerializer::deserialize<PlatformConfig>(configJson, config);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to deserialize platform config: " << e.what();
    throw;
  }
  XLOG(DBG2) << SimpleJSONSerializer::serialize<std::string>(config);

  verifyPlatformNameMatches(*config.platformName(), platformNameFromBios);

  if (!ConfigValidator().isValid(config)) {
    XLOG(ERR) << "Invalid platform config";
    throw std::runtime_error("Invalid platform config");
  }
  config_ = config;
  return config;
}

bool ConfigUtils::hasConfigChanged() {
  auto currentHash = computeHash(getConfig());

  auto storedHashResult = pFsUtils_->getStringFileContent(kConfigHashFile);
  if (!storedHashResult.has_value()) {
    // In the first run of this change, we will not have config hash file of
    // running config. In this case align with the existing behavior of the
    // service (of not reloading just because only config changed) and dont
    // reload kmods. Once units have the config hash file, we can change the
    // behavior to trigger relaoding of kmods if config hash file is missing.
    XLOG(INFO)
        << "No stored config hash found, can't determine if config has changed";
    return false;
  }

  auto storedHash = folly::trimWhitespace(storedHashResult.value()).toString();
  XLOGF(INFO, "Config hash: stored={}, current={}", storedHash, currentHash);

  bool changed = (currentHash != storedHash);
  if (changed) {
    auto storedBuildInfo = pFsUtils_->getStringFileContent(kBuildInfoFile);
    auto currentBuildInfo = helpers::getBuildSummary();
    XLOGF(INFO, "Build info (previous): {}", storedBuildInfo.value_or("none"));
    XLOGF(INFO, "Build info (current): {}", currentBuildInfo);
  }
  return changed;
}

void ConfigUtils::storeConfigHash() {
  auto hash = computeHash(getConfig());

  if (!pFsUtils_->createDirectories(
          std::filesystem::path(kConfigHashFile).parent_path())) {
    XLOG(ERR) << "Failed to create directory for config hash file";
    return;
  }

  if (!pFsUtils_->writeStringToFile(hash + "\n", kConfigHashFile)) {
    XLOGF(ERR, "Failed to write config hash to {}", kConfigHashFile);
    return;
  }

  auto buildInfo = helpers::getBuildSummary();
  if (!pFsUtils_->writeStringToFile(buildInfo + "\n", kBuildInfoFile)) {
    XLOGF(ERR, "Failed to write build info to {}", kBuildInfoFile);
    return;
  }

  XLOGF(INFO, "Stored config hash: {}", hash);
}
} // namespace facebook::fboss::platform::platform_manager
