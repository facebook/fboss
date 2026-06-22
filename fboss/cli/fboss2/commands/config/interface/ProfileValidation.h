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
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
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
 * call validateProfile() or validateBatch() for each port without repeated
 * Thrift calls.
 */
class ProfileValidator {
 public:
  // ── Constructors ──────────────────────────────────────────────────────────

  // Production constructor: queries Agent (platformMapping, portInfo) and
  // QSFP service.
  explicit ProfileValidator(const HostInfo& hostInfo);

  // Test-only constructor (two-arg): provides platformMapping and
  // qsfpSupportedProfiles directly; currentPortInfo defaults to empty (no
  // parent-subsumption or subsumed-port detection).
  ProfileValidator(
      cfg::PlatformMapping platformMapping,
      std::map<std::string, std::vector<cfg::PortProfileID>>
          qsfpSupportedProfiles);

  // Test-only constructor (three-arg): provides all data directly.
  // Use this to exercise validateBatch() with controlled currentPortInfo.
  ProfileValidator(
      cfg::PlatformMapping platformMapping,
      std::map<std::string, std::vector<cfg::PortProfileID>>
          qsfpSupportedProfiles,
      std::map<int32_t, PortInfoThrift> currentPortInfo);

  ~ProfileValidator();

  // ── Static helpers ────────────────────────────────────────────────────────

  // Parse profile string (case-insensitive) to cfg::PortProfileID enum.
  // Throws std::invalid_argument on unrecognized string.
  static cfg::PortProfileID parseProfile(const std::string& profileStr);

  // ── Per-port (single) API ─────────────────────────────────────────────────

  // Validate that portName supports profileStr.
  // Returns the validated PortProfileID.
  // Throws std::invalid_argument if not supported.
  // Throws std::runtime_error if port not found.
  // NOTE: This path does NOT run family-level (51T/25T/12T) rules since it
  //       only sees one port at a time.  Use validateBatch() for that.
  cfg::PortProfileID validateProfile(
      const std::string& portName,
      const std::string& profileStr);

  // Return the speed for a profile on the given port from PlatformMapping.
  // Returns cfg::PortSpeed::DEFAULT if not determinable.
  cfg::PortSpeed getProfileSpeed(PortID portId, cfg::PortProfileID profile);

  // ── Batch API ─────────────────────────────────────────────────────────────

  // Describes a single port→profile change request.
  struct ProfileChange {
    std::string portName;
    cfg::PortProfileID profile;
  };

  // Aggregated result of a batch validation.
  struct ValidationResult {
    std::vector<std::string> errors; // non-empty → reject the whole batch
    std::vector<std::string> warnings; // subsumed-port notices, etc.
    std::vector<PortID> subsumedPorts; // ports that will be subsumed
  };

  // Validate a batch of port→profile changes.
  //
  // Steps:
  //   1. Per-port supportedProfiles membership check for each change.
  //   2. Parent-subsumption check: for each change, resolve the port's parent
  //      (controllingPort).  If the parent's effective profile — its target
  //      profile when the parent is also in this batch, else its current live
  //      profile — lists the changed port in subsumedPorts, the port cannot be
  //      configured → append to errors.
  //   3. Populate subsumedPorts/warnings from PlatformPortConfig.subsumedPorts
  //      for the changed ports (ports each change will itself subsume).
  //
  // Returns ValidationResult.  The caller is responsible for checking
  // result.errors before applying any mutations.
  ValidationResult validateBatch(const std::vector<ProfileChange>& changes);

 private:
  cfg::PlatformMapping platformMapping_;
  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpSupportedProfiles_;
  std::unique_ptr<PlatformMapping> platformMappingObj_;
  std::map<std::string, std::vector<cfg::PortProfileID>> allPortProfiles_;

  // Populated by the production constructor via:
  //   agentClient->sync_getAllPortInfo(currentPortInfo_)
  // getAllPortInfo() returns map<i32, PortInfoThrift> with speedMbps (i64,
  // field 2) and profileID (string, field 19).  We use this typed call
  // instead of getCurrentStateJSON() for directness and type safety.
  std::map<int32_t, PortInfoThrift> currentPortInfo_;

  std::vector<cfg::PortProfileID> getSupportedProfiles(
      const std::string& portName);
};

} // namespace facebook::fboss
