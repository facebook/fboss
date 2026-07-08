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
#include <map>
#include <optional>
#include <set>
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

ProfileValidator::~ProfileValidator() = default;

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

void ProfileValidator::validateProfile(
    const std::string& portName,
    cfg::PortProfileID requestedProfile) {
  // Get supported profiles for this port
  auto supportedProfiles = getSupportedProfiles(portName);

  // Check if requested profile is in the supported list
  auto it = std::find(
      supportedProfiles.begin(), supportedProfiles.end(), requestedProfile);
  if (it != supportedProfiles.end()) {
    return;
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
          apache::thrift::util::enumNameSafe(requestedProfile),
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

ProfileValidator::ProfileChangeResult ProfileValidator::validateProfileChange(
    const cfg::SwitchConfig& currentConfig,
    const std::vector<std::string>& portNames,
    cfg::PortProfileID profile) {
  // Requested name -> id (throws FbossError if not a platform port).
  std::map<PortID, std::string> requested;
  for (const auto& name : portNames) {
    requested.emplace(platformMappingObj_->getPortID(name), name);
  }

  // Current config profile by logicalID — the effective profile of any port we
  // are not changing.
  std::map<PortID, cfg::PortProfileID> configProfile;
  for (const auto& port : *currentConfig.ports()) {
    configProfile.emplace(PortID(*port.logicalID()), *port.profileID());
  }
  auto effectiveProfile = [&](PortID id) -> std::optional<cfg::PortProfileID> {
    if (auto it = requested.find(id); it != requested.end()) {
      return profile;
    }
    if (auto it = configProfile.find(id); it != configProfile.end()) {
      return it->second;
    }
    return std::nullopt; // absent from config and not requested
  };

  // 1) Each requested port must support the profile.
  for (const auto& [id, name] : requested) {
    validateProfile(name, profile);
  }

  // 2) No requested port may be subsumed by its controlling port's effective
  //    profile (catches input-vs-input AND input-vs-existing-controlling-port).
  for (const auto& [id, name] : requested) {
    const PortID controlling(
        *platformMappingObj_->getPlatformPort(static_cast<int32_t>(id))
             .mapping()
             ->controllingPort());
    if (controlling == id) {
      continue; // self-controlling: it consumes others, not itself
    }
    const auto cProfile = effectiveProfile(controlling);
    if (!cProfile.has_value()) {
      continue; // controlling port inactive -> no subsumption
    }
    const auto subsumed =
        platformMappingObj_->getSubsumedPorts(controlling, *cProfile);
    if (std::find(subsumed.begin(), subsumed.end(), id) != subsumed.end()) {
      const auto cName =
          platformMappingObj_->getPortNameByPortId(controlling)
              .value_or(std::to_string(static_cast<uint32_t>(controlling)));
      throw std::invalid_argument(
          fmt::format(
              "Cannot apply profile {} to port {}: its controlling port {} is "
              "at profile {} which subsumes {}. Change {} first or drop {} from "
              "the request.",
              apache::thrift::util::enumNameSafe(profile),
              name,
              cName,
              apache::thrift::util::enumNameSafe(*cProfile),
              name,
              cName,
              name));
    }
  }

  // 3) Ports to remove: union of what each requested port subsumes at the new
  //    profile (skip HYPER_PORT, per coop patchers.py). Only include ports that
  //    are actually present in the config — a subsumed port that isn't in the
  //    config needs no removal and must not be reported as auto-removed.
  std::set<PortID> toRemove;
  for (const auto& [id, name] : requested) {
    const auto& entry =
        platformMappingObj_->getPlatformPort(static_cast<int32_t>(id));
    if (*entry.mapping()->portType() == cfg::PortType::HYPER_PORT) {
      continue;
    }
    for (PortID sp : platformMappingObj_->getSubsumedPorts(id, profile)) {
      if (configProfile.count(sp) > 0) {
        toRemove.insert(sp);
      }
    }
  }

  return ProfileChangeResult{
      std::vector<PortID>(toRemove.begin(), toRemove.end())};
}

} // namespace facebook::fboss
