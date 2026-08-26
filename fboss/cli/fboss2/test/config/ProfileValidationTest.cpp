/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/commands/config/interface/ProfileValidation.h"

using namespace ::testing;

namespace facebook::fboss {

namespace {
constexpr auto kWide = cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N;
constexpr auto kNarrow = cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91;

void addProfile(
    cfg::PlatformPortEntry& entry,
    cfg::PortProfileID profile,
    const std::vector<int32_t>& subsumed = {}) {
  cfg::PlatformPortConfig pc;
  if (!subsumed.empty()) {
    pc.subsumedPorts() = subsumed;
  }
  entry.supportedProfiles()[profile] = pc;
}

cfg::PlatformPortEntry
makeEntry(int id, std::string name, int controllingPort) {
  cfg::PlatformPortEntry entry;
  entry.mapping()->id() = id;
  entry.mapping()->name() = std::move(name);
  entry.mapping()->controllingPort() = controllingPort;
  return entry;
}

// Three ports in one group controlled by port 1 (eth1/1/1). At the wide
// profile, the controlling port subsumes ports 2 and 3.
cfg::PlatformMapping makeMapping() {
  cfg::PlatformMapping pm;

  auto a = makeEntry(1, "eth1/1/1", /*controllingPort*/ 1);
  addProfile(a, kNarrow);
  addProfile(a, kWide, /*subsumed*/ {2, 3});
  pm.ports()[1] = a;

  auto b = makeEntry(2, "eth1/1/2", /*controllingPort*/ 1);
  addProfile(b, kNarrow);
  addProfile(b, kWide); // so the conflict path (not "unsupported") is exercised
  pm.ports()[2] = b;

  auto c = makeEntry(3, "eth1/1/3", /*controllingPort*/ 1);
  addProfile(c, kNarrow); // only narrow -> used for the unsupported case
  pm.ports()[3] = c;

  return pm;
}

cfg::Port makeCfgPort(int id, cfg::PortProfileID profile) {
  cfg::Port p;
  p.logicalID() = id;
  p.name() = std::string("eth1/1/") + std::to_string(id);
  p.profileID() = profile;
  p.ingressVlan() = 1;
  return p;
}

cfg::SwitchConfig makeConfig(
    const std::vector<std::pair<int, cfg::PortProfileID>>& ports) {
  cfg::SwitchConfig config;
  config.defaultVlan() = 1;
  for (const auto& [id, profile] : ports) {
    config.ports()->push_back(makeCfgPort(id, profile));
  }
  return config;
}

ProfileValidator makeValidator() {
  return ProfileValidator(
      makeMapping(), std::map<std::string, std::vector<cfg::PortProfileID>>{});
}
} // namespace

// Changing the controlling port to the wide profile returns the ports it
// subsumes.
TEST(ProfileValidationTest, returnsSubsumedPortsForControllingPort) {
  auto validator = makeValidator();
  auto config = makeConfig({{1, kNarrow}, {2, kNarrow}, {3, kNarrow}});

  auto result = validator.validateProfileChange(config, {"eth1/1/1"}, kWide);

  const std::vector<PortID> expected = {PortID(2), PortID(3)};
  EXPECT_EQ(result.portsToRemove, expected);
}

// Subsumed ports that are not present in the config are not reported for
// removal (removing them would be a no-op and must not be surfaced to the
// user).
TEST(ProfileValidationTest, skipsSubsumedPortsAbsentFromConfig) {
  auto validator = makeValidator();
  // Only the controlling port exists; ports 2 and 3 were previously removed.
  auto config = makeConfig({{1, kNarrow}});

  auto result = validator.validateProfileChange(config, {"eth1/1/1"}, kWide);

  EXPECT_TRUE(result.portsToRemove.empty());
}

// A port cannot be changed while its (unchanged) controlling port still
// subsumes it.
TEST(ProfileValidationTest, rejectsPortSubsumedByExistingControllingPort) {
  auto validator = makeValidator();
  // Controlling port eth1/1/1 is already at the wide profile (subsumes 2, 3).
  auto config = makeConfig({{1, kWide}});

  try {
    validator.validateProfileChange(config, {"eth1/1/2"}, kNarrow);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("eth1/1/2"));
    EXPECT_THAT(e.what(), HasSubstr("eth1/1/1")); // names the controlling port
  }
}

// A port cannot be changed when another port in the same request subsumes it.
TEST(ProfileValidationTest, rejectsInputSubsumedByAnotherInput) {
  auto validator = makeValidator();
  auto config = makeConfig({{1, kNarrow}, {2, kNarrow}});

  try {
    validator.validateProfileChange(config, {"eth1/1/1", "eth1/1/2"}, kWide);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("eth1/1/2"));
    EXPECT_THAT(e.what(), HasSubstr("eth1/1/1"));
  }
}

// A profile the port does not support is rejected before any subsumption logic.
TEST(ProfileValidationTest, rejectsUnsupportedProfile) {
  auto validator = makeValidator();
  auto config = makeConfig({{3, kNarrow}});

  try {
    validator.validateProfileChange(config, {"eth1/1/3"}, kWide);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Invalid profile"));
    EXPECT_THAT(e.what(), HasSubstr("eth1/1/3"));
  }
}

} // namespace facebook::fboss
