/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 */

/**
 * Unit tests for ProfileValidator::validateBatch():
 *   - subsumed-port detection (ports the changed port itself subsumes)
 *   - parent-subsumption check (a port whose parent's profile subsumes it
 *     cannot be configured)
 *
 * All tests are pure (no Thrift I/O) — they use the helper constructor that
 * accepts pre-built data structures.
 */

#include "fboss/cli/fboss2/commands/config/interface/ProfileValidation.h"
#include <gtest/gtest.h>
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"

using namespace facebook::fboss;

// ════════════════════════════════════════════════════════════════════════════
// ProfileValidator::validateBatch — subsumed-port warning
// ════════════════════════════════════════════════════════════════════════════

TEST(ProfileValidatorBatch, SubsumedPortsPopulatedInResult) {
  // Build a minimal PlatformMapping with one port whose 400G profile has a
  // subsumedPorts list containing port id 42.

  cfg::PlatformMapping pm;

  cfg::PlatformPortEntry entry;
  cfg::PlatformPortMapping mapping;
  mapping.name() = "eth1/1/1";
  mapping.id() = 1;
  entry.mapping() = mapping;

  // Minimal profile config: 400G with subsumedPorts = [42]
  cfg::PlatformPortConfig prof400g;
  prof400g.subsumedPorts() = {42};
  entry.supportedProfiles() = {
      {cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL, prof400g}};

  pm.ports() = {{1, entry}};

  // Also build allPortProfiles so that getSupportedProfiles() succeeds.
  // We provide a qsfpSupportedProfiles map as the override.
  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpProfiles;
  qsfpProfiles["eth1/1/1"] = {
      cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL};

  // Build currentPortInfo for live-state lookups.
  PortInfoThrift portInfo;
  portInfo.portId() = 1;
  portInfo.name() = "eth1/1/1";
  portInfo.speedMbps() = 100000; // currently 100G
  portInfo.profileID() = "PROFILE_100G_4_NRZ_RS528_COPPER";

  std::map<int32_t, PortInfoThrift> currentPortInfo;
  currentPortInfo[1] = portInfo;

  ProfileValidator validator(pm, qsfpProfiles, currentPortInfo);

  std::vector<ProfileValidator::ProfileChange> changes = {
      {"eth1/1/1", cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL}};

  auto result = validator.validateBatch(changes);

  EXPECT_TRUE(result.errors.empty())
      << "Unexpected error: "
      << (result.errors.empty() ? "" : result.errors[0]);
  ASSERT_EQ(result.subsumedPorts.size(), 1u);
  EXPECT_EQ(result.subsumedPorts[0], PortID(42));
  ASSERT_EQ(result.warnings.size(), 1u);
  EXPECT_NE(result.warnings[0].find("subsume"), std::string::npos);
}

TEST(ProfileValidatorBatch, NoSubsumedPortsWhenProfileHasNone) {
  cfg::PlatformMapping pm;

  cfg::PlatformPortEntry entry;
  cfg::PlatformPortMapping mapping;
  mapping.name() = "eth1/1/1";
  mapping.id() = 1;
  entry.mapping() = mapping;

  // Profile with no subsumedPorts.
  cfg::PlatformPortConfig prof100g;
  entry.supportedProfiles() = {
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER, prof100g}};
  pm.ports() = {{1, entry}};

  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpProfiles;
  qsfpProfiles["eth1/1/1"] = {
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER};

  PortInfoThrift portInfo;
  portInfo.portId() = 1;
  portInfo.name() = "eth1/1/1";
  portInfo.speedMbps() = 400000;
  portInfo.profileID() = "PROFILE_400G_8_PAM4_RS544X2N_OPTICAL";

  std::map<int32_t, PortInfoThrift> currentPortInfo;
  currentPortInfo[1] = portInfo;

  ProfileValidator validator(pm, qsfpProfiles, currentPortInfo);

  std::vector<ProfileValidator::ProfileChange> changes = {
      {"eth1/1/1", cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER}};

  auto result = validator.validateBatch(changes);

  EXPECT_TRUE(result.errors.empty());
  EXPECT_TRUE(result.subsumedPorts.empty());
  EXPECT_TRUE(result.warnings.empty());
}

// ════════════════════════════════════════════════════════════════════════════
// Atomicity: validateBatch() itself never mutates — callers that only mutate
// when result.errors is empty are safe.
// ════════════════════════════════════════════════════════════════════════════

TEST(ProfileValidatorBatch, ErrorResultContainsNoMutation) {
  // Profile not in supportedProfiles → error, not panic or partial mutation.
  cfg::PlatformMapping pm;

  cfg::PlatformPortEntry entry;
  cfg::PlatformPortMapping mapping;
  mapping.name() = "eth1/1/1";
  mapping.id() = 1;
  entry.mapping() = mapping;

  // Only 100G is supported.
  cfg::PlatformPortConfig prof100g;
  entry.supportedProfiles() = {
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER, prof100g}};
  pm.ports() = {{1, entry}};

  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpProfiles;
  qsfpProfiles["eth1/1/1"] = {
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER};

  PortInfoThrift portInfo;
  portInfo.portId() = 1;
  portInfo.name() = "eth1/1/1";
  portInfo.speedMbps() = 100000;
  portInfo.profileID() = "PROFILE_100G_4_NRZ_RS528_COPPER";

  std::map<int32_t, PortInfoThrift> currentPortInfo;
  currentPortInfo[1] = portInfo;

  ProfileValidator validator(pm, qsfpProfiles, currentPortInfo);

  // Request 400G which is NOT in the supported list.
  std::vector<ProfileValidator::ProfileChange> changes = {
      {"eth1/1/1", cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL}};

  auto result = validator.validateBatch(changes);

  // Must have errors. Caller pattern: only mutate if errors is empty, so a
  // non-empty errors list is the signal to skip all mutations. validateBatch
  // itself never mutates, and the populated lists below would be irrelevant.
  EXPECT_FALSE(result.errors.empty())
      << "Caller must NOT apply mutations when errors is non-empty";
  EXPECT_TRUE(result.subsumedPorts.empty());
}

// ════════════════════════════════════════════════════════════════════════════
// ProfileValidator::validateBatch — parent/child (breakout) handling
// ════════════════════════════════════════════════════════════════════════════

namespace {

// Build a platform mapping with a parent (id=1) and a sibling child (id=2)
// sharing controllingPort=1. The parent supports two profiles:
//   - 100G_4 ("full" profile) → subsumes port 2.
//   - 50G_2  ("half" profile) → does NOT subsume; port 2 is a usable
//     breakout child.
// The child (id=2) optionally supports the half profile.
cfg::PlatformMapping makeBreakoutPlatformMapping(bool childSupportsHalf) {
  cfg::PlatformMapping pm;

  // Parent — id=1, controllingPort=1.
  cfg::PlatformPortEntry parent;
  cfg::PlatformPortMapping parentMapping;
  parentMapping.name() = "eth1/1/1";
  parentMapping.id() = 1;
  parentMapping.controllingPort() = 1;
  parent.mapping() = parentMapping;

  cfg::PlatformPortConfig fullCfg;
  fullCfg.subsumedPorts() = {2};
  cfg::PlatformPortConfig halfCfg; // no subsumed ports
  parent.supportedProfiles() = {
      {cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER, fullCfg},
      {cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER, halfCfg}};

  // Child — id=2, controllingPort=1 (sibling of parent).
  cfg::PlatformPortEntry child;
  cfg::PlatformPortMapping childMapping;
  childMapping.name() = "eth1/1/5";
  childMapping.id() = 2;
  childMapping.controllingPort() = 1;
  childMapping.scope() = cfg::Scope::LOCAL;
  child.mapping() = childMapping;

  if (childSupportsHalf) {
    cfg::PlatformPortConfig childHalfCfg;
    child.supportedProfiles() = {
        {cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER, childHalfCfg}};
  } else {
    // Child only supports a different profile.
    cfg::PlatformPortConfig childOtherCfg;
    child.supportedProfiles() = {
        {cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER, childOtherCfg}};
  }

  pm.ports() = {{1, parent}, {2, child}};
  return pm;
}

} // namespace

TEST(ProfileValidatorBatch, DownshiftDoesNotAutoCreateSiblings) {
  // Parent currently at full (100G_4 subsumes port 2); request half (50G_2)
  // which does not subsume. We no longer auto-create the freed sibling; the
  // change applies only to the requested port, leaving sibling lanes idle.

  auto pm = makeBreakoutPlatformMapping(/*childSupportsHalf=*/true);

  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpProfiles;
  qsfpProfiles["eth1/1/1"] = {
      cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER};

  PortInfoThrift parentInfo;
  parentInfo.portId() = 1;
  parentInfo.name() = "eth1/1/1";
  parentInfo.speedMbps() = 100000;
  parentInfo.profileID() = "PROFILE_100G_4_NRZ_RS528_COPPER";

  std::map<int32_t, PortInfoThrift> currentPortInfo;
  currentPortInfo[1] = parentInfo;

  ProfileValidator validator(pm, qsfpProfiles, currentPortInfo);

  std::vector<ProfileValidator::ProfileChange> changes = {
      {"eth1/1/1", cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER}};

  auto result = validator.validateBatch(changes);

  EXPECT_TRUE(result.errors.empty())
      << "Unexpected error: "
      << (result.errors.empty() ? "" : result.errors[0]);
  EXPECT_TRUE(result.subsumedPorts.empty());
}

TEST(ProfileValidatorBatch, SubsumedPortsPopulatedOnUpshift) {
  // Request the full profile which subsumes port 2. Port 2 must appear in
  // subsumedPorts so the caller can strip it from the config.

  auto pm = makeBreakoutPlatformMapping(/*childSupportsHalf=*/true);

  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpProfiles;
  qsfpProfiles["eth1/1/1"] = {
      cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER};

  PortInfoThrift parentInfo;
  parentInfo.portId() = 1;
  parentInfo.name() = "eth1/1/1";
  parentInfo.speedMbps() = 50000;
  parentInfo.profileID() = "PROFILE_50G_2_NRZ_RS528_COPPER";

  std::map<int32_t, PortInfoThrift> currentPortInfo;
  currentPortInfo[1] = parentInfo;

  ProfileValidator validator(pm, qsfpProfiles, currentPortInfo);

  std::vector<ProfileValidator::ProfileChange> changes = {
      {"eth1/1/1", cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER}};

  auto result = validator.validateBatch(changes);

  EXPECT_TRUE(result.errors.empty());
  ASSERT_EQ(result.subsumedPorts.size(), 1u);
  EXPECT_EQ(result.subsumedPorts[0], PortID(2));
}

TEST(ProfileValidatorBatch, ChildRejectedWhenParentSubsumesIt) {
  // Parent (eth1/1/1) is live at 100G_4, which subsumes child port 2
  // (eth1/1/5). Configuring the child must be rejected: it does not exist as
  // an independent port while the parent subsumes it.

  auto pm = makeBreakoutPlatformMapping(/*childSupportsHalf=*/true);

  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpProfiles;
  qsfpProfiles["eth1/1/5"] = {
      cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER};

  // Parent is live at the full profile (subsumes the child).
  PortInfoThrift parentInfo;
  parentInfo.portId() = 1;
  parentInfo.name() = "eth1/1/1";
  parentInfo.speedMbps() = 100000;
  parentInfo.profileID() = "PROFILE_100G_4_NRZ_RS528_COPPER";

  std::map<int32_t, PortInfoThrift> currentPortInfo;
  currentPortInfo[1] = parentInfo;

  ProfileValidator validator(pm, qsfpProfiles, currentPortInfo);

  std::vector<ProfileValidator::ProfileChange> changes = {
      {"eth1/1/5", cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER}};

  auto result = validator.validateBatch(changes);

  ASSERT_FALSE(result.errors.empty());
  EXPECT_NE(result.errors[0].find("subsumed by"), std::string::npos);
  EXPECT_NE(result.errors[0].find("eth1/1/1"), std::string::npos);
}

TEST(ProfileValidatorBatch, ChildAllowedWhenParentDownshiftedInSameBatch) {
  // Parent is live at 100G_4 (subsumes child), but the SAME batch downshifts
  // the parent to 50G_2 (no subsume) and configures the child. The batch
  // overlay must use the parent's target profile, so the child is allowed.

  auto pm = makeBreakoutPlatformMapping(/*childSupportsHalf=*/true);

  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpProfiles;
  qsfpProfiles["eth1/1/1"] = {
      cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER};
  qsfpProfiles["eth1/1/5"] = {
      cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER};

  PortInfoThrift parentInfo;
  parentInfo.portId() = 1;
  parentInfo.name() = "eth1/1/1";
  parentInfo.speedMbps() = 100000;
  parentInfo.profileID() = "PROFILE_100G_4_NRZ_RS528_COPPER";

  std::map<int32_t, PortInfoThrift> currentPortInfo;
  currentPortInfo[1] = parentInfo;

  ProfileValidator validator(pm, qsfpProfiles, currentPortInfo);

  std::vector<ProfileValidator::ProfileChange> changes = {
      {"eth1/1/1", cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER},
      {"eth1/1/5", cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER}};

  auto result = validator.validateBatch(changes);

  EXPECT_TRUE(result.errors.empty())
      << "Unexpected error: "
      << (result.errors.empty() ? "" : result.errors[0]);
}

TEST(ProfileValidatorBatch, ChildAllowedWhenParentNotInPlatformMapping) {
  // A child whose controllingPort points to a port id absent from
  // PlatformMapping cannot have a resolvable parent, so the parent-subsumption
  // check must skip it (not error). Exercises the "parent not found" branch.
  cfg::PlatformMapping pm;

  cfg::PlatformPortEntry child;
  cfg::PlatformPortMapping childMapping;
  childMapping.name() = "eth1/1/5";
  childMapping.id() = 2;
  childMapping.controllingPort() = 999; // no such port in the mapping
  child.mapping() = childMapping;
  cfg::PlatformPortConfig childCfg; // no subsumed ports
  child.supportedProfiles() = {
      {cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER, childCfg}};
  pm.ports() = {{2, child}};

  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpProfiles;
  qsfpProfiles["eth1/1/5"] = {
      cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER};

  ProfileValidator validator(pm, qsfpProfiles, /*currentPortInfo=*/{});

  std::vector<ProfileValidator::ProfileChange> changes = {
      {"eth1/1/5", cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER}};

  auto result = validator.validateBatch(changes);

  EXPECT_TRUE(result.errors.empty())
      << "Unexpected error: "
      << (result.errors.empty() ? "" : result.errors[0]);
  EXPECT_TRUE(result.subsumedPorts.empty());
}

TEST(ProfileValidatorBatch, ChildAllowedWhenParentProfileUndeterminable) {
  // The parent exists in PlatformMapping but is absent from live state and not
  // in the batch, so its effective profile cannot be determined. The check
  // must not block the child (graceful degradation), exercising the
  // "can't determine parent's profile" branch.
  auto pm = makeBreakoutPlatformMapping(/*childSupportsHalf=*/true);

  std::map<std::string, std::vector<cfg::PortProfileID>> qsfpProfiles;
  qsfpProfiles["eth1/1/5"] = {
      cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER};

  // currentPortInfo intentionally omits the parent (id 1).
  ProfileValidator validator(pm, qsfpProfiles, /*currentPortInfo=*/{});

  std::vector<ProfileValidator::ProfileChange> changes = {
      {"eth1/1/5", cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER}};

  auto result = validator.validateBatch(changes);

  EXPECT_TRUE(result.errors.empty())
      << "Unexpected error: "
      << (result.errors.empty() ? "" : result.errors[0]);
}
