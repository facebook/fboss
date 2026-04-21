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
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

/**
 * ProfileValidator provides profile validation logic for port configuration.
 * It queries both the platform hardware capabilities and optics (QSFP)
 * to ensure the requested profile is supported.
 *
 * Construct once (which queries Agent and QSFP services via Thrift), then
 * call validateProfile() for each port without repeated Thrift calls.
 */
class ProfileValidator {
 public:
  explicit ProfileValidator(const HostInfo& hostInfo);

  ProfileValidator(
      cfg::PlatformMapping platformMapping,
      std::map<std::string, std::vector<cfg::PortProfileID>>
          qsfpSupportedProfiles);

  // Parse profile string (case-insensitive) to cfg::PortProfileID enum.
  // Throws std::invalid_argument on unrecognized string.
  static cfg::PortProfileID parseProfile(const std::string& profileStr);

  // Validate that portName supports profileStr.
  // Returns the validated PortProfileID.
  // PROFILE_DEFAULT always passes (no hardware check).
  // Throws std::invalid_argument if not supported.
  // Throws std::runtime_error if port not found.
  cfg::PortProfileID validateProfile(
      const std::string& portName,
      const std::string& profileStr);

  // Return the speed for a profile on the given port from PlatformMapping.
  // Returns cfg::PortSpeed::DEFAULT if not determinable.
  cfg::PortSpeed getProfileSpeed(PortID portId, cfg::PortProfileID profile);

 private:
  cfg::PlatformMapping platformMapping_;
  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpSupportedProfiles_;

  std::vector<cfg::PortProfileID> getSupportedProfiles(
      const std::string& portName);
};

} // namespace facebook::fboss
