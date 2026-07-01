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
#include <iostream>
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

// ── Constructors ──────────────────────────────────────────────────────────

ProfileValidator::ProfileValidator(const HostInfo& hostInfo) {
  auto agentClient =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  agentClient->sync_getPlatformMapping(platformMapping_);

  // Fetch live port state for the parent-subsumption and subsumed-port
  // checks.  getAllPortInfo() is the correct typed call here: it returns
  // map<i32, PortInfoThrift> with speedMbps (i64, field 2) and profileID
  // (string, field 19) — no need for getCurrentStateJSON().
  agentClient->sync_getAllPortInfo(currentPortInfo_);

  try {
    auto qsfpClient =
        utils::createClient<apache::thrift::Client<QsfpService>>(hostInfo);
    qsfpClient->sync_getAllPortSupportedProfiles(qsfpSupportedProfiles_, true);
  } catch (const std::exception& e) {
    // QsfpService unavailable — fall back to PlatformMapping in
    // getSupportedProfiles(). Surface why, since the fallback validates
    // against hardware capability only (not the installed optic).
    std::cerr << "Warning: QsfpService unavailable (" << e.what()
              << "); validating profiles against PlatformMapping only"
              << std::endl;
  }

  platformMappingObj_ = std::make_unique<PlatformMapping>(platformMapping_);
  allPortProfiles_ = platformMappingObj_->getAllPortProfiles();
}

ProfileValidator::ProfileValidator(
    cfg::PlatformMapping platformMapping,
    std::map<std::string, std::vector<cfg::PortProfileID>>
        qsfpSupportedProfiles)
    : platformMapping_(std::move(platformMapping)),
      qsfpSupportedProfiles_(std::move(qsfpSupportedProfiles)),
      platformMappingObj_(std::make_unique<PlatformMapping>(platformMapping_)),
      allPortProfiles_(platformMappingObj_->getAllPortProfiles()) {}

ProfileValidator::ProfileValidator(
    cfg::PlatformMapping platformMapping,
    std::map<std::string, std::vector<cfg::PortProfileID>>
        qsfpSupportedProfiles,
    std::map<int32_t, PortInfoThrift> currentPortInfo)
    : platformMapping_(std::move(platformMapping)),
      qsfpSupportedProfiles_(std::move(qsfpSupportedProfiles)),
      platformMappingObj_(std::make_unique<PlatformMapping>(platformMapping_)),
      allPortProfiles_(platformMappingObj_->getAllPortProfiles()),
      currentPortInfo_(std::move(currentPortInfo)) {}

ProfileValidator::~ProfileValidator() = default;

// ── Static helpers ────────────────────────────────────────────────────────

// static
cfg::PortProfileID ProfileValidator::parseProfile(
    const std::string& profileStr) {
  cfg::PortProfileID result{};
  if (!apache::thrift::TEnumTraits<cfg::PortProfileID>::findValue(
          toUpper(profileStr), &result)) {
    throw std::invalid_argument(
        fmt::format(
            "Invalid profile value '{}': not a valid PortProfileID. "
            "Valid values include: PROFILE_10G_1_NRZ_NOFEC, "
            "PROFILE_100G_4_NRZ_RS528_COPPER, PROFILE_400G_8_PAM4_RS544X2N_OPTICAL, "
            "... (see PortProfileID enum in switch_config.thrift)",
            profileStr));
  }
  return result;
}

// ── Private helpers ────────────────────────────────────────────────────────

std::vector<cfg::PortProfileID> ProfileValidator::getSupportedProfiles(
    const std::string& portName) {
  // First try QSFP service (optics-aware profiles)
  auto qsfpIt = qsfpSupportedProfiles_.find(portName);
  if (qsfpIt != qsfpSupportedProfiles_.end() && !qsfpIt->second.empty()) {
    return qsfpIt->second;
  }

  // Fall back to platform mapping (hardware capabilities)
  auto it = allPortProfiles_.find(portName);
  if (it == allPortProfiles_.end()) {
    throw std::runtime_error(fmt::format("Port {} not found", portName));
  }
  if (it->second.empty()) {
    throw std::invalid_argument(
        fmt::format("No supported profiles found for port {}", portName));
  }
  return it->second;
}

// ── Per-port (single) API ─────────────────────────────────────────────────

cfg::PortProfileID ProfileValidator::validateProfile(
    const std::string& portName,
    const std::string& profileStr) {
  // Parse first — throws std::invalid_argument on bad string
  cfg::PortProfileID requestedProfile = parseProfile(profileStr);

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
  auto profileConfig = platformMappingObj_->getPortProfileConfig(
      PlatformPortProfileConfigMatcher(profile, portId));
  if (profileConfig.has_value()) {
    return static_cast<cfg::PortSpeed>(
        static_cast<int64_t>(profileConfig->speed().value()));
  }
  return cfg::PortSpeed::DEFAULT;
}

// ── Batch API ─────────────────────────────────────────────────────────────

ProfileValidator::ValidationResult ProfileValidator::validateBatch(
    const std::vector<ProfileChange>& changes) {
  ValidationResult result;

  // ── Step 1: Per-port profile membership check ──────────────────────────
  for (const auto& change : changes) {
    try {
      auto supported = getSupportedProfiles(change.portName);
      auto it = std::find(supported.begin(), supported.end(), change.profile);
      if (it == supported.end()) {
        std::vector<std::string> supportedNames;
        supportedNames.reserve(supported.size());
        for (const auto& p : supported) {
          supportedNames.emplace_back(apache::thrift::util::enumNameSafe(p));
        }
        result.errors.emplace_back(
            fmt::format(
                "Invalid profile '{}' for port {}. Supported profiles: [{}]",
                apache::thrift::util::enumNameSafe(change.profile),
                change.portName,
                folly::join(", ", supportedNames)));
      }
    } catch (const std::exception& e) {
      result.errors.emplace_back(e.what());
    }
  }

  // ── Step 2: Parent-subsumption check ───────────────────────────────────
  // A port cannot be configured while its parent (controllingPort) holds a
  // profile that subsumes it.  The parent's effective profile is its target
  // profile when the parent is itself in this batch, otherwise its current
  // live profile (batch overlay).

  // name → target profile, for the batch overlay.
  std::map<std::string, cfg::PortProfileID> targetByName;
  for (const auto& c : changes) {
    targetByName[c.portName] = c.profile;
  }

  // name → portId, from PlatformMapping (authoritative for controllingPort).
  std::map<std::string, int32_t> idByName;
  for (const auto& [portIdInt, entry] : *platformMapping_.ports()) {
    idByName[*entry.mapping()->name()] = portIdInt;
  }

  // portId → current profile enum, parsed from live state. nullopt if the port
  // is absent from live state or its profileID string doesn't parse.
  auto currentProfileOf =
      [&](int32_t portId) -> std::optional<cfg::PortProfileID> {
    auto it = currentPortInfo_.find(portId);
    if (it == currentPortInfo_.end()) {
      return std::nullopt;
    }
    const std::string& pidStr = *it->second.profileID();
    cfg::PortProfileID pid{};
    if (!pidStr.empty() &&
        apache::thrift::TEnumTraits<cfg::PortProfileID>::findValue(
            pidStr, &pid)) {
      return pid;
    }
    return std::nullopt;
  };

  for (const auto& change : changes) {
    auto idIt = idByName.find(change.portName);
    if (idIt == idByName.end()) {
      continue; // unknown port — Step 1 already surfaced this
    }
    const int32_t portId = idIt->second;

    const auto& entry = platformMapping_.ports()->at(portId);
    const int32_t parentId = *entry.mapping()->controllingPort();
    if (parentId == portId) {
      continue; // port is its own parent — nothing above it can subsume it
    }

    auto parentIt = platformMapping_.ports()->find(parentId);
    if (parentIt == platformMapping_.ports()->end()) {
      continue;
    }
    const auto& parentEntry = parentIt->second;
    const std::string& parentName = *parentEntry.mapping()->name();

    // Parent's effective profile: batch target overrides live state.
    std::optional<cfg::PortProfileID> parentProfile;
    auto tIt = targetByName.find(parentName);
    if (tIt != targetByName.end()) {
      parentProfile = tIt->second;
    } else {
      parentProfile = currentProfileOf(parentId);
    }
    if (!parentProfile.has_value()) {
      continue; // can't determine parent's profile — don't block
    }

    const auto& parentProfiles = *parentEntry.supportedProfiles();
    auto profIt = parentProfiles.find(*parentProfile);
    if (profIt == parentProfiles.end()) {
      continue;
    }
    if (!profIt->second.subsumedPorts().has_value()) {
      continue;
    }
    const auto& subsumed = *profIt->second.subsumedPorts();
    if (std::find(subsumed.begin(), subsumed.end(), portId) != subsumed.end()) {
      result.errors.emplace_back(
          fmt::format(
              "Cannot configure port {} to profile {}: it is subsumed by "
              "parent port {} (profile {}). Reconfigure the parent port first.",
              change.portName,
              apache::thrift::util::enumNameSafe(change.profile),
              parentName,
              apache::thrift::util::enumNameSafe(*parentProfile)));
    }
  }

  // ── Step 3: Subsumed ports ──────────────────────────────────────────────
  for (const auto& change : changes) {
    // Resolve portId from PlatformMapping (via idByName, built in Step 2) — the
    // same authoritative source the subsumedPorts lookup below uses. Resolving
    // from live state instead would skip ports present in PlatformMapping but
    // absent from getAllPortInfo() (e.g. a currently-subsumed port), leaving
    // their subsumed neighbours in the config and aborting the agent on apply.
    auto idIt = idByName.find(change.portName);
    if (idIt == idByName.end()) {
      continue;
    }
    const int32_t portIdInt = idIt->second;

    auto pmIt = platformMapping_.ports()->find(portIdInt);
    if (pmIt == platformMapping_.ports()->end()) {
      continue;
    }
    const auto& supportedProfiles = *pmIt->second.supportedProfiles();
    auto profIt = supportedProfiles.find(change.profile);
    if (profIt == supportedProfiles.end()) {
      continue;
    }

    const auto& profConfig = profIt->second;
    if (profConfig.subsumedPorts().has_value()) {
      for (int32_t subPort : *profConfig.subsumedPorts()) {
        result.subsumedPorts.emplace_back(subPort);
        result.warnings.emplace_back(
            fmt::format(
                "Port {} (profile {}) subsumes port {}: will be disabled",
                change.portName,
                apache::thrift::util::enumNameSafe(change.profile),
                subPort));
      }
    }
  }

  return result;
}

} // namespace facebook::fboss
