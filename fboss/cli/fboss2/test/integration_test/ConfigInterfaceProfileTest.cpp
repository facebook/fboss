/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/**
 * End-to-end tests for 'fboss2-dev config interface <name> profile <P>'.
 *
 * Test candidates are derived from the device's PlatformMapping (fetched via
 * thrift) rather than hardcoded profile strings: we locate a controlling port
 * and, from its supported profiles, pick profiles that (1) it does not support,
 * (2) it supports without subsuming any port, and (3) it supports and that
 * subsume at least one port currently present in the config. This keeps the
 * tests portable across platforms and exercises the subsumed-port removal path.
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 */

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

using namespace facebook::fboss;

class ConfigInterfaceProfileTest : public Fboss2IntegrationTest {
 protected:
  // A concrete profile-change to exercise, derived from the platform mapping.
  struct Candidate {
    std::string portName; // controlling port
    std::string currentProfile; // enum name currently on the port
    std::string targetProfile; // enum name to change to
    std::vector<std::string>
        subsumedPresent; // present ports the change removes
  };

  struct PresentPort {
    std::string name;
    std::string profileId; // enum name string
  };

  /**
   * Set a profile on an interface and commit the config.
   * Only commits if the CLI set succeeds (mirrors setInterfaceSpeed).
   */
  Result setInterfaceProfile(
      const std::string& interfaceName,
      const std::string& profile) {
    auto result =
        runCli({"config", "interface", interfaceName, "profile", profile});
    if (result.exitCode == 0) {
      commitConfig();
    }
    return result;
  }

  // Build a PlatformMapping from the running agent.
  PlatformMapping fetchPlatformMapping() const {
    HostInfo hostInfo("localhost");
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    cfg::PlatformMapping thriftMapping;
    client->sync_getPlatformMapping(thriftMapping);
    return PlatformMapping(thriftMapping);
  }

  // Ports currently present in the agent (subsumed/removed ports are absent),
  // keyed by logical id.
  std::map<int32_t, PresentPort> fetchPresentPorts() const {
    HostInfo hostInfo("localhost");
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::map<int32_t, PortInfoThrift> portEntries;
    client->sync_getAllPortInfo(portEntries);
    std::map<int32_t, PresentPort> present;
    for (const auto& [id, info] : portEntries) {
      present[id] = PresentPort{*info.name(), *info.profileID()};
    }
    return present;
  }

  // Any port currently present in the agent, regardless of its name. Returns
  // nullopt if the agent reports no ports.
  std::optional<PresentPort> findAnyPresentPort() const {
    auto present = fetchPresentPorts();
    if (present.empty()) {
      return std::nullopt;
    }
    return present.begin()->second;
  }

  // Poll until the named port disappears from getAllPortInfo (i.e. it was
  // removed as a subsumed port). Returns false if it is still present after the
  // timeout.
  bool waitForPortAbsent(const std::string& portName) const {
    for (int attempt = 0; attempt < 15; ++attempt) {
      try {
        getPortRunningInfo(portName);
      } catch (const std::runtime_error&) {
        return true;
      }
      /* sleep override */ std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    return false;
  }

  // A present port + a valid PortProfileID it does NOT support.
  std::optional<Candidate> findUnsupportedProfileCandidate() const {
    auto mapping = fetchPlatformMapping();
    auto present = fetchPresentPorts();
    const auto& platformPorts = mapping.getPlatformPorts();
    for (const auto& [id, pp] : present) {
      auto it = platformPorts.find(id);
      if (it == platformPorts.end()) {
        continue;
      }
      std::set<cfg::PortProfileID> supported;
      for (const auto& [profileId, unused] : *it->second.supportedProfiles()) {
        supported.insert(profileId);
      }
      for (const auto profileId :
           apache::thrift::TEnumTraits<cfg::PortProfileID>::values) {
        if (profileId == cfg::PortProfileID::PROFILE_DEFAULT) {
          continue;
        }
        if (supported.find(profileId) == supported.end()) {
          return Candidate{
              pp.name,
              pp.profileId,
              apache::thrift::util::enumNameSafe(profileId),
              {}};
        }
      }
    }
    return std::nullopt;
  }

  // A present, self-controlling port + a supported profile (!= current)
  // that subsumes nothing.
  std::optional<Candidate> findNonSubsumingCandidate() const {
    auto mapping = fetchPlatformMapping();
    auto present = fetchPresentPorts();
    auto groups = utility::getSubsidiaryPortIDs(mapping.getPlatformPorts());
    for (const auto& [controlling, unused] : groups) {
      int32_t cid = static_cast<int32_t>(controlling);
      auto pit = present.find(cid);
      if (pit == present.end()) {
        continue;
      }
      const auto& entry = mapping.getPlatformPort(cid);
      for (const auto& [profileId, unused2] : *entry.supportedProfiles()) {
        auto name = apache::thrift::util::enumNameSafe(profileId);
        if (name == pit->second.profileId) {
          continue;
        }
        if (mapping.getSubsumedPorts(controlling, profileId).empty()) {
          return Candidate{pit->second.name, pit->second.profileId, name, {}};
        }
      }
    }
    return std::nullopt;
  }

  // A present, self-controlling port + a supported profile (!= current)
  // that subsumes at least one currently-present port.
  std::optional<Candidate> findSubsumingCandidate() const {
    auto mapping = fetchPlatformMapping();
    auto present = fetchPresentPorts();
    auto groups = utility::getSubsidiaryPortIDs(mapping.getPlatformPorts());
    for (const auto& [controlling, unused] : groups) {
      int32_t cid = static_cast<int32_t>(controlling);
      auto pit = present.find(cid);
      if (pit == present.end()) {
        continue;
      }
      const auto& entry = mapping.getPlatformPort(cid);
      for (const auto& [profileId, unused2] : *entry.supportedProfiles()) {
        auto name = apache::thrift::util::enumNameSafe(profileId);
        if (name == pit->second.profileId) {
          continue;
        }
        std::vector<std::string> subsumedPresent;
        for (auto sp : mapping.getSubsumedPorts(controlling, profileId)) {
          auto sit = present.find(static_cast<int32_t>(sp));
          if (sit != present.end()) {
            subsumedPresent.push_back(sit->second.name);
          }
        }
        if (!subsumedPresent.empty()) {
          return Candidate{
              pit->second.name, pit->second.profileId, name, subsumedPresent};
        }
      }
    }
    return std::nullopt;
  }
};

// Case 1: Changing a port to a valid profile it does not support is rejected
// per-port (before commit), leaving the running config unchanged.
TEST_F(ConfigInterfaceProfileTest, ChangeToUnsupportedProfileRejected) {
  auto cand = findUnsupportedProfileCandidate();
  if (!cand) {
    GTEST_SKIP() << "No present port with an unsupported profile found";
  }
  XLOG(INFO) << "[Case 1] port=" << cand->portName
             << " unsupported target=" << cand->targetProfile
             << " (current=" << cand->currentProfile << ")";

  auto result = runCli(
      {"config", "interface", cand->portName, "profile", cand->targetProfile});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for an unsupported profile";
  EXPECT_THAT(result.stderr, ::testing::HasSubstr("Invalid profile"));

  // Not committed → agent still reports the original profile.
  auto info = getPortRunningInfo(cand->portName);
  EXPECT_EQ(info.profileId, cand->currentProfile);
  XLOG(INFO) << "TEST PASSED";
}

// Case 2: Changing a controlling port to a supported profile that subsumes no
// ports succeeds and is reflected by the agent.
TEST_F(ConfigInterfaceProfileTest, ChangeProfileWithoutSubsumingSucceeds) {
  auto cand = findNonSubsumingCandidate();
  if (!cand) {
    GTEST_SKIP()
        << "No self-controlling port with a non-subsuming alternate profile";
  }
  XLOG(INFO) << "[Case 2] port=" << cand->portName << " "
             << cand->currentProfile << " -> " << cand->targetProfile;

  auto result = setInterfaceProfile(cand->portName, cand->targetProfile);
  ASSERT_EQ(result.exitCode, 0) << result.stderr;

  auto after =
      waitForPortRunningInfo(cand->portName, [&cand](const auto& info) {
        return info.profileId == cand->targetProfile;
      });
  EXPECT_EQ(after.profileId, cand->targetProfile);

  // Restore original profile.
  auto restore = setInterfaceProfile(cand->portName, cand->currentProfile);
  EXPECT_EQ(restore.exitCode, 0) << restore.stderr;
  XLOG(INFO) << "TEST PASSED";
}

// Case 3: Changing a controlling port to a supported profile that subsumes a
// currently-present port succeeds, and the subsumed port is removed from the
// config (verified via the agent).
TEST_F(ConfigInterfaceProfileTest, ChangeProfileSubsumesAndRemovesPorts) {
  auto cand = findSubsumingCandidate();
  if (!cand) {
    GTEST_SKIP()
        << "No self-controlling port with a subsuming profile over a present port";
  }
  XLOG(INFO) << "[Case 3] port=" << cand->portName << " "
             << cand->currentProfile << " -> " << cand->targetProfile
             << "; expected to remove: "
             << folly::join(", ", cand->subsumedPresent);

  auto result = setInterfaceProfile(cand->portName, cand->targetProfile);
  ASSERT_EQ(result.exitCode, 0) << result.stderr;
  // The CLI reports the ports it auto-removed.
  EXPECT_THAT(
      result.stdout, ::testing::HasSubstr("auto-removed subsumed port"));

  auto after =
      waitForPortRunningInfo(cand->portName, [&cand](const auto& info) {
        return info.profileId == cand->targetProfile;
      });
  EXPECT_EQ(after.profileId, cand->targetProfile);

  // Each subsumed port that was present should now be gone from the agent.
  for (const auto& sub : cand->subsumedPresent) {
    EXPECT_TRUE(waitForPortAbsent(sub))
        << sub << " should be removed after a subsuming profile change";
  }

  // Best-effort restore of the controlling port's profile. Note: removal-only
  // semantics mean the subsumed ports are NOT re-added by this restore.
  setInterfaceProfile(cand->portName, cand->currentProfile);
  XLOG(INFO) << "TEST PASSED";
}

// Setting a completely invalid profile string is rejected before commit
// (parseProfile path, distinct from the per-port unsupported-profile path).
TEST_F(ConfigInterfaceProfileTest, SetInvalidProfileFails) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  auto port = findAnyPresentPort();
  if (!port) {
    GTEST_SKIP() << "No present port found";
  }
  XLOG(INFO) << "  Using interface: " << port->name;

  XLOG(INFO)
      << "[Step 2] Setting invalid profile 'PROFILE_COMPLETELY_BOGUS_9999'...";
  auto result =
      setInterfaceProfile(port->name, "PROFILE_COMPLETELY_BOGUS_9999");
  XLOG(INFO) << "  Exit code: " << result.exitCode;

  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit code for invalid profile";
  EXPECT_NE(result.stderr.find("not a valid PortProfileID"), std::string::npos)
      << "Expected 'not a valid PortProfileID' in stderr, got: "
      << result.stderr;

  XLOG(INFO) << "TEST PASSED";
}

// The command accepts multiple space-separated port names.
TEST_F(ConfigInterfaceProfileTest, SetProfileMultiInterface) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  auto port = findAnyPresentPort();
  if (!port) {
    GTEST_SKIP() << "No present port found";
  }
  XLOG(INFO) << "  Using interface: " << port->name;

  XLOG(INFO) << "[Step 2] Reading current profile from agent...";
  auto info = getPortRunningInfo(port->name);
  XLOG(INFO) << "  Current profile: " << info.profileId;

  if (info.profileId == "PROFILE_DEFAULT") {
    GTEST_SKIP() << "Port has PROFILE_DEFAULT; no specific profile to set";
  }

  // Re-apply the current profile on a multi-port list (same port listed twice,
  // passed as separate CLI tokens so InterfacesConfig resolves each one).
  XLOG(INFO) << "[Step 3] Setting " << info.profileId
             << " on two-port list: " << port->name << " " << port->name;
  auto result = runCli(
      {"config",
       "interface",
       port->name,
       port->name,
       "profile",
       info.profileId});
  ASSERT_EQ(result.exitCode, 0) << result.stderr;
  commitConfig();

  // Verify via direct thrift with wait-retry
  XLOG(INFO) << "[Step 4] Verifying via agent thrift...";
  auto after = waitForPortRunningInfo(port->name, [&info](const auto& i) {
    return i.profileId == info.profileId;
  });
  EXPECT_EQ(after.profileId, info.profileId)
      << "Agent should report " << info.profileId
      << " after multi-interface set";

  XLOG(INFO) << "TEST PASSED";
}
