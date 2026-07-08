/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

class PlatformMapping;

/**
 * ProfileValidator provides profile validation logic for port configuration.
 * It queries both the platform hardware capabilities and optics (QSFP)
 * to ensure the requested profile is supported.
 *
 * Construct once (which queries Agent and QSFP services via Thrift), then
 * reuse across ports without repeated Thrift calls. validateProfileChange() is
 * the primary entry point for a profile change: it runs the full validation
 * (per-port support via validateProfile(), plus controlling-port subsumption
 * checks) and returns the ports that must be removed.
 */
class ProfileValidator {
 public:
  explicit ProfileValidator(const HostInfo& hostInfo);

  ProfileValidator(
      cfg::PlatformMapping platformMapping,
      std::map<std::string, std::vector<cfg::PortProfileID>>
          qsfpSupportedProfiles);

  ~ProfileValidator();

  // Parse profile string (case-insensitive) to cfg::PortProfileID enum.
  // Throws std::invalid_argument on unrecognized string.
  static cfg::PortProfileID parseProfile(const std::string& profileStr);

  // Validate that portName supports `profile`.
  // Throws std::invalid_argument if not supported.
  // Throws std::runtime_error if port not found.
  void validateProfile(const std::string& portName, cfg::PortProfileID profile);

  // Return the speed for a profile on the given port from PlatformMapping.
  // Returns cfg::PortSpeed::DEFAULT if not determinable.
  cfg::PortSpeed getProfileSpeed(PortID portId, cfg::PortProfileID profile);

  struct ProfileChangeResult {
    // Ports that must be removed from the config because they are subsumed by
    // the requested ports at the new profile.
    std::vector<PortID> portsToRemove;
  };

  // Validate applying `profile` to `portNames` against `currentConfig`.
  // Throws std::invalid_argument (with a user-facing message) if a port does
  // not support the profile, or if a requested port would be subsumed by its
  // controlling port's effective profile (the new profile if the controlling
  // port is also requested, else its current-config profile). On success
  // returns the subsumed ports to remove.
  ProfileChangeResult validateProfileChange(
      const cfg::SwitchConfig& currentConfig,
      const std::vector<std::string>& portNames,
      cfg::PortProfileID profile);

 private:
  cfg::PlatformMapping platformMapping_;
  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpSupportedProfiles_;
  std::unique_ptr<PlatformMapping> platformMappingObj_;
  std::map<std::string, std::vector<cfg::PortProfileID>> allPortProfiles_;

  std::vector<cfg::PortProfileID> getSupportedProfiles(
      const std::string& portName);
};

} // namespace facebook::fboss
