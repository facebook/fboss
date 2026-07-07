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
// A platform may reuse another platform's config (config alias); the canonical
// name resolves the alias so the aliased platform is accepted.
void verifyPlatformNameMatches(
    const std::string& platformNameInConfig,
    const std::string& platformNameFromBios) {
  std::string platformNameInConfigUpper(platformNameInConfig);
  std::transform(
      platformNameInConfigUpper.begin(),
      platformNameInConfigUpper.end(),
      platformNameInConfigUpper.begin(),
      ::toupper);

  if (platformNameInConfigUpper ==
      ConfigLib::canonicalConfigPlatformName(platformNameFromBios)) {
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
    XLOG(INFO)
        << "No stored config hash found, can't determine if config has changed. Assuming config has changed.";
    return true;
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
