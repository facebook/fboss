/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/SpeedValidation.h"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <folly/String.h>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"

namespace facebook::fboss {

namespace {

// Convert lowercase speed string to uppercase for case-insensitive comparison
std::string normalizeSpeedString(const std::string& speedStr) {
  std::string normalized = speedStr;
  std::transform(
      normalized.begin(), normalized.end(), normalized.begin(), [](char c) {
        return std::toupper(c);
      });
  return normalized;
}

// Check if a string is a valid integer
bool isInteger(const std::string& str) {
  if (str.empty()) {
    return false;
  }
  return std::all_of(
      str.begin(), str.end(), [](char c) { return std::isdigit(c); });
}

} // namespace

SpeedValidator::SpeedValidator(const HostInfo& hostInfo) {
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

SpeedValidator::SpeedValidator(
    cfg::PlatformMapping platformMapping,
    std::map<std::string, std::vector<cfg::PortProfileID>>
        qsfpSupportedProfiles)
    : platformMapping_(std::move(platformMapping)),
      qsfpSupportedProfiles_(std::move(qsfpSupportedProfiles)) {}

// Get supported profiles for a specific port
std::vector<cfg::PortProfileID> SpeedValidator::getSupportedProfiles(
    const std::string& portName) {
  // First try QSFP service (optics-aware profiles)
  auto qsfpIt = qsfpSupportedProfiles_.find(portName);
  if (qsfpIt != qsfpSupportedProfiles_.end() && !qsfpIt->second.empty()) {
    return qsfpIt->second;
  }

  // Fall back to platform mapping (hardware capabilities)
  // Find the port ID by name in the platform mapping
  const auto& portMap = *platformMapping_.ports();
  PortID portId(0);
  bool found = false;
  for (const auto& [id, portEntry] : portMap) {
    if (*portEntry.mapping()->name() == portName) {
      portId = PortID(id);
      found = true;
      break;
    }
  }

  if (!found) {
    throw std::runtime_error(fmt::format("Port {} not found", portName));
  }

  const auto& portConfig = portMap.at(static_cast<int32_t>(portId));
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

// Parse speed string to integer (returns 0 for "auto" or "0")
int64_t SpeedValidator::parseSpeedToMbps(const std::string& speedStr) {
  std::string normalized = normalizeSpeedString(speedStr);
  if (normalized == "AUTO") {
    return 0; // Special value for auto-negotiation
  }

  if (!isInteger(speedStr)) {
    throw std::invalid_argument(
        fmt::format("Invalid speed value: {}", speedStr));
  }

  int64_t speedMbps = folly::to<int64_t>(speedStr);
  // Treat "0" as auto
  if (speedMbps == 0) {
    return 0;
  }

  return speedMbps;
}

// Get speed for a profile from platform mapping
int64_t SpeedValidator::getProfileSpeed(
    PortID portId,
    cfg::PortProfileID profile) {
  // Use PlatformMapping to get the actual speed from profile config
  PlatformMapping pm(platformMapping_);

  auto profileConfig = pm.getPortProfileConfig(
      PlatformPortProfileConfigMatcher(profile, portId));

  if (profileConfig.has_value()) {
    return static_cast<int64_t>(profileConfig->speed().value());
  }

  // Fall back to 0 if we can't find the profile config
  return 0;
}

// Format error message with supported speeds
std::string SpeedValidator::formatErrorMessage(
    PortID portId,
    const std::string& portName,
    const std::string& speedStr,
    const std::vector<cfg::PortProfileID>& supportedProfiles) {
  std::vector<int64_t> supportedSpeeds;
  for (const auto& profile : supportedProfiles) {
    int64_t speed = getProfileSpeed(portId, profile);
    if (speed > 0) {
      supportedSpeeds.push_back(speed);
    }
  }

  // Remove duplicates and sort
  std::sort(supportedSpeeds.begin(), supportedSpeeds.end());
  supportedSpeeds.erase(
      std::unique(supportedSpeeds.begin(), supportedSpeeds.end()),
      supportedSpeeds.end());

  std::string speedList = folly::join(", ", supportedSpeeds);
  return fmt::format(
      "Invalid speed {} for port {}. Supported speeds (in Mbps): {}",
      speedStr,
      portName,
      speedList);
}

// Public API: Validate speed
std::vector<cfg::PortProfileID> SpeedValidator::validateSpeed(
    const std::string& portName,
    const std::string& speedStr) {
  // Get all supported profiles for this port
  auto supportedProfiles = getSupportedProfiles(portName);

  // Find the port ID for profile speed lookup
  const auto& portMap = *platformMapping_.ports();
  PortID portId(0);
  for (const auto& [id, portEntry] : portMap) {
    if (*portEntry.mapping()->name() == portName) {
      portId = PortID(id);
      break;
    }
  }

  // Parse the requested speed
  int64_t requestedSpeed = parseSpeedToMbps(speedStr);

  // "auto" speed always valid - return all supported profiles
  if (requestedSpeed == 0) {
    return supportedProfiles;
  }

  // Find matching profiles
  std::vector<cfg::PortProfileID> matchingProfiles;
  for (const auto& profile : supportedProfiles) {
    int64_t profileSpeed = getProfileSpeed(portId, profile);
    if (profileSpeed == requestedSpeed) {
      matchingProfiles.push_back(profile);
    }
  }

  // If no matching profiles, throw error with supported speeds
  if (matchingProfiles.empty()) {
    throw std::invalid_argument(
        formatErrorMessage(portId, portName, speedStr, supportedProfiles));
  }

  return matchingProfiles;
}

// Helper: Convert speed in Mbps to cfg::PortSpeed enum
cfg::PortSpeed SpeedValidator::speedToPortSpeedEnum(int64_t speedMbps) {
  // Handle auto (0 or negative values)
  if (speedMbps <= 0) {
    return cfg::PortSpeed::DEFAULT;
  }

  // Cast to PortSpeed enum
  // The enum values are defined as speed in Mbps (e.g., SPEED_100G = 100000)
  return static_cast<cfg::PortSpeed>(speedMbps);
}

// Public API: Parse speed string to cfg::PortSpeed enum
cfg::PortSpeed SpeedValidator::parseSpeed(const std::string& speedStr) {
  int64_t speedMbps = parseSpeedToMbps(speedStr);
  return speedToPortSpeedEnum(speedMbps);
}

} // namespace facebook::fboss
