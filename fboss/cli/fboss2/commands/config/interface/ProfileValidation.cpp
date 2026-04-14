/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 */

#include "fboss/cli/fboss2/commands/config/interface/ProfileValidation.h"

#include <fmt/format.h>
#include <folly/String.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

namespace facebook::fboss {

namespace {
std::string toUpper(const std::string& value) {
  std::string upper = value;
  std::transform(upper.begin(), upper.end(), upper.begin(), [](char c) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  });
  return upper;
}
} // namespace

ProfileValidator::ProfileValidator(const HostInfo& hostInfo) {
  auto agentClient =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  agentClient->sync_getPlatformMapping(platformMapping_);

  try {
    auto qsfpClient =
        utils::createClient<apache::thrift::Client<QsfpService>>(hostInfo);
    qsfpClient->sync_getAllPortSupportedProfiles(qsfpSupportedProfiles_, true);
  } catch (const std::exception&) {
    // QsfpService unavailable — fall back to PlatformMapping in
    // getSupportedProfiles()
  }
}

ProfileValidator::ProfileValidator(
    cfg::PlatformMapping platformMapping,
    std::map<std::string, std::vector<cfg::PortProfileID>>
        qsfpSupportedProfiles)
    : platformMapping_(std::move(platformMapping)),
      qsfpSupportedProfiles_(std::move(qsfpSupportedProfiles)) {}

// static
cfg::PortProfileID ProfileValidator::parseProfile(
    const std::string& profileStr) {
  cfg::PortProfileID result{};
  if (!apache::thrift::TEnumTraits<cfg::PortProfileID>::findValue(
          toUpper(profileStr), &result)) {
    throw std::invalid_argument(
        fmt::format(
            "Invalid profile value '{}': not a valid PortProfileID. "
            "Valid values include: PROFILE_DEFAULT, PROFILE_10G_1_NRZ_NOFEC, "
            "PROFILE_100G_4_NRZ_RS528_COPPER, PROFILE_400G_8_PAM4_RS544X2N_OPTICAL, "
            "... (61 values total, see PortProfileID enum in switch_config.thrift)",
            profileStr));
  }
  return result;
}

std::vector<cfg::PortProfileID> ProfileValidator::getSupportedProfiles(
    const std::string& portName) {
  // First try QSFP service (optics-aware profiles)
  auto qsfpIt = qsfpSupportedProfiles_.find(portName);
  if (qsfpIt != qsfpSupportedProfiles_.end() && !qsfpIt->second.empty()) {
    return qsfpIt->second;
  }

  // Fall back to platform mapping (hardware capabilities)
  const auto& portMap = *platformMapping_.ports();
  bool found = false;
  int32_t foundId = 0;
  for (const auto& [id, portEntry] : portMap) {
    if (*portEntry.mapping()->name() == portName) {
      foundId = id;
      found = true;
      break;
    }
  }

  if (!found) {
    throw std::runtime_error(fmt::format("Port {} not found", portName));
  }

  const auto& portConfig = portMap.at(foundId);
  if (portConfig.supportedProfiles()->empty()) {
    throw std::invalid_argument(
        fmt::format("No supported profiles found for port {}", portName));
  }

  // Extract profile IDs from the map (keys only)
  const auto& profileMap = portConfig.supportedProfiles().value();
  std::vector<cfg::PortProfileID> profiles;
  profiles.reserve(profileMap.size());
  for (const auto& [profileId, _] : profileMap) {
    profiles.push_back(profileId);
  }

  return profiles;
}

cfg::PortProfileID ProfileValidator::validateProfile(
    const std::string& portName,
    const std::string& profileStr) {
  // Parse first — throws std::invalid_argument on bad string
  cfg::PortProfileID requestedProfile = parseProfile(profileStr);

  // PROFILE_DEFAULT always passes — skip hardware check
  if (requestedProfile == cfg::PortProfileID::PROFILE_DEFAULT) {
    return requestedProfile;
  }

  // Get supported profiles for this port
  auto supportedProfiles = getSupportedProfiles(portName);

  // Check if requested profile is in the supported list
  auto it = std::find(
      supportedProfiles.begin(), supportedProfiles.end(), requestedProfile);
  if (it != supportedProfiles.end()) {
    return requestedProfile;
  }

  // Build supported profile names for error message
  std::vector<std::string> supportedNames;
  supportedNames.reserve(supportedProfiles.size());
  for (const auto& profile : supportedProfiles) {
    supportedNames.push_back(apache::thrift::util::enumNameSafe(profile));
  }

  throw std::invalid_argument(
      fmt::format(
          "Invalid profile '{}' for port {}. Supported profiles: [{}]",
          profileStr,
          portName,
          folly::join(", ", supportedNames)));
}

cfg::PortSpeed ProfileValidator::getProfileSpeed(
    PortID portId,
    cfg::PortProfileID profile) {
  PlatformMapping pm(platformMapping_);
  auto profileConfig = pm.getPortProfileConfig(
      PlatformPortProfileConfigMatcher(profile, portId));
  if (profileConfig.has_value()) {
    return static_cast<cfg::PortSpeed>(
        static_cast<int64_t>(profileConfig->speed().value()));
  }
  return cfg::PortSpeed::DEFAULT;
}

} // namespace facebook::fboss
