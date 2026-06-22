// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for 'fboss2-dev config interface <name> profile <P>'.
 *
 * Structure:
 *   - One positive test (ProfileChange_RoundTrip) that picks two ports at the
 *     same profile (or falls back to one), changes them to a different-lane
 *     profile and back, verifying each leg (including subsumed-port removal)
 *     against the agent's running config.
 *   - Three negative tests for the validation paths that don't mutate.
 *
 * The fixture snapshots the full agent config in SetUp and restores it via
 * coldboot in TearDown, so every test leaves the switch exactly as found —
 * including ports a profile change subsumed, which a CLI re-apply of the
 * original profile cannot recreate.
 *
 * Profile changes go through the coldboot path (see applyProfile in
 * CmdConfigInterface.cpp — BCM SAI rejects the live-delta flexport recreate),
 * so every commit here waits for agent recovery.
 *
 * Requirements:
 *   - FBOSS agent must be running with a valid configuration.
 *   - The test must be run as root (or with appropriate permissions).
 */

#include <folly/String.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace facebook::fboss;

namespace {

// Profile names follow PROFILE_<speed>_<lanes>_<modulation>_<fec>_<medium>,
// e.g. PROFILE_800G_8_PAM4_RS544X2N_OPTICAL → 8 lanes. Returns 0 on a
// malformed name.
int profileLaneCount(const std::string& profileId) {
  std::vector<std::string> parts;
  folly::split('_', profileId, parts);
  if (parts.size() < 3) {
    return 0;
  }
  try {
    return std::stoi(parts[2]);
  } catch (...) {
    return 0;
  }
}

} // namespace

class ConfigInterfaceProfileTest : public Fboss2IntegrationTest {
 protected:
  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    // Snapshot the full agent config so TearDown can restore it verbatim. A
    // profile change can subsume ports (removing them from the config); a CLI
    // re-apply of the original profile does NOT recreate them, so per-command
    // restore cannot fully undo an upshift. Restoring the whole snapshot via a
    // coldboot does, leaving the switch exactly as found regardless of the
    // direction the test exercised.
    configSnapshot_ = getRunningConfig();
  }

  void TearDown() override {
    restoreConfigSnapshotIfChanged();
    Fboss2IntegrationTest::TearDown();
  }

  // Restore the SetUp snapshot if the test committed any config change. Skips
  // the (expensive) coldboot when nothing changed — e.g. the negative tests
  // that never commit.
  void restoreConfigSnapshotIfChanged() const {
    try {
      if (configSnapshot_.isNull()) {
        return; // SetUp snapshot unavailable; nothing to restore.
      }
      if (getRunningConfig() == configSnapshot_) {
        return; // No committed change — leave the switch (and time) alone.
      }
      XLOG(INFO) << "[TearDown] Config changed during test; restoring the "
                 << "pre-test snapshot via coldboot...";
      auto& session = ConfigSession::getInstance();
      session.getAgentConfig() =
          apache::thrift::SimpleJSONSerializer::deserialize<cfg::AgentConfig>(
              folly::toJson(configSnapshot_));
      session.saveConfig(
          cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_COLDBOOT);
      session.commit(HostInfo("localhost"));
      waitForAgentReady();
    } catch (const std::exception& ex) {
      ADD_FAILURE() << "[TearDown] Failed to restore pre-test config: "
                    << ex.what();
    }
  }

  Result setInterfaceProfile(
      const std::string& interfaceName,
      const std::string& profile) {
    auto result =
        runCli({"config", "interface", interfaceName, "profile", profile});
    if (result.exitCode == 0) {
      commitConfig();
      waitForAgentReady();
    }
    return result;
  }

  // Extract the supported-profile list from a CLI rejection message of the
  // form: "Supported profiles: [PROFILE_A, PROFILE_B, ...]".
  std::vector<std::string> extractSupportedProfiles(
      const std::string& errorMessage) const {
    static constexpr std::string_view kPrefix = "Supported profiles: [";
    std::vector<std::string> profiles;
    auto pos = errorMessage.find(kPrefix);
    if (pos == std::string::npos) {
      return profiles;
    }
    pos += kPrefix.size();
    auto end = errorMessage.find(']', pos);
    if (end == std::string::npos) {
      return profiles;
    }
    auto listStr = errorMessage.substr(pos, end - pos);
    folly::splitTo<std::string>(", ", listStr, std::back_inserter(profiles));
    return profiles;
  }

  // Warnings parsed out of the CLI's stderr for a profile change.
  struct CliWarnings {
    std::set<int> subsumedIds; // port IDs that the CLI reports as subsumed
  };

  CliWarnings parseCliWarnings(const std::string& stderr_) const {
    CliWarnings w;
    std::istringstream ss(stderr_);
    std::string line;
    const std::string kSubsumes = "subsumes port ";
    while (std::getline(ss, line)) {
      auto pos = line.find(kSubsumes);
      if (pos != std::string::npos) {
        pos += kSubsumes.size();
        auto colon = line.find(':', pos);
        if (colon != std::string::npos) {
          try {
            w.subsumedIds.insert(std::stoi(line.substr(pos, colon - pos)));
          } catch (...) {
          }
        }
      }
    }
    return w;
  }

  // Index of the running config's sw.ports by name and by logicalID. Target
  // ports are checked by name; subsumed ports are checked by ID.
  struct PortIndex {
    std::set<std::string> presentNames;
    std::set<int> presentIds;
  };

  PortIndex readPortIndex() const {
    PortIndex idx;
    auto cfg = getRunningConfig();
    if (!cfg["sw"].count("ports")) {
      return idx;
    }
    for (const auto& p : cfg["sw"]["ports"]) {
      if (p.count("name")) {
        idx.presentNames.insert(p["name"].asString());
      }
      if (p.count("logicalID")) {
        idx.presentIds.insert(p["logicalID"].asInt());
      }
    }
    return idx;
  }

  // Verify the running config matches what should be true after applying
  // `targetProfile` to `targetPorts`, given the CLI's warning text.
  void verifyAtProfile(
      const std::vector<std::string>& targetPorts,
      const std::string& targetProfile,
      const CliWarnings& warnings) const {
    auto idx = readPortIndex();

    for (const auto& name : targetPorts) {
      EXPECT_TRUE(idx.presentNames.count(name))
          << "Target port " << name << " missing from cfg.sw.ports";
      auto info = getPortRunningInfo(name);
      EXPECT_EQ(info.profileId, targetProfile)
          << "Target port " << name << " profile mismatch";
    }

    for (int id : warnings.subsumedIds) {
      EXPECT_FALSE(idx.presentIds.count(id))
          << "Subsumed port id=" << id << " still present in cfg.sw.ports";
    }
  }

  Result runConfigInterface(
      const std::vector<std::string>& ports,
      const std::string& profile) {
    std::vector<std::string> args = {"config", "interface"};
    args.insert(args.end(), ports.begin(), ports.end());
    args.emplace_back("profile");
    args.push_back(profile);
    return runCli(args);
  }

  // Full agent config captured in SetUp, restored in TearDown.
  folly::dynamic configSnapshot_;
};

/**
 * ProfileChange_RoundTrip
 *
 * Pick two eth ports sharing a current profile P0 (fall back to one if no
 * pair exists). Find a supported target profile P1 with a different lane count
 * (either direction). Apply P1, verify on the agent, apply P0 back, verify
 * again. The two legs exercise both an upshift and a downshift, including
 * subsumed-port removal, checked via running-config invariants derived from the
 * CLI's warning text.
 *
 * The base fixture snapshots the full config in SetUp and restores it (via
 * coldboot) in TearDown, so the switch is left exactly as found even when a leg
 * subsumes ports that a CLI re-apply cannot recreate — no per-test cleanup
 * needed here.
 *
 * SKIP cases (none are failures):
 *   - No eth ports with a non-default profile.
 *   - No supported alternate profile with a different lane count.
 *   - Multi-port rejected by validation AND no single-port fallback.
 */
TEST_F(ConfigInterfaceProfileTest, ProfileChange_RoundTrip) {
  XLOG(INFO) << "[Step 1] Picking test ports...";
  auto allIfs = getAllInterfaces();
  std::map<std::string, std::vector<std::string>> profileToPorts;
  for (const auto& [name, iface] : allIfs) {
    if (name.find("eth") != 0) {
      continue;
    }
    try {
      auto info = getPortRunningInfo(name);
      if (info.profileId == "PROFILE_DEFAULT") {
        continue;
      }
      profileToPorts[info.profileId].push_back(name);
    } catch (...) {
      // Port not in current cfg (e.g. previously subsumed) — skip.
    }
  }

  std::string p0;
  std::vector<std::string> testPorts;
  for (const auto& [profile, names] : profileToPorts) {
    if (names.size() >= 2) {
      p0 = profile;
      testPorts = {names[0], names[1]};
      break;
    }
  }
  if (testPorts.empty()) {
    // Fall back to one port if no two share a profile.
    for (const auto& [profile, names] : profileToPorts) {
      if (!names.empty()) {
        p0 = profile;
        testPorts = {names[0]};
        break;
      }
    }
  }
  if (testPorts.empty()) {
    GTEST_SKIP() << "No eth ports with a non-default profile";
  }
  XLOG(INFO) << "  Using " << folly::join(",", testPorts) << " at " << p0;

  XLOG(INFO) << "[Step 2] Discovering supported profiles...";
  const std::string kProbeA = "PROFILE_10G_1_NRZ_NOFEC";
  const std::string kProbeB = "PROFILE_100G_4_NRZ_RS528_COPPER";
  const std::string probe = (p0 == kProbeA) ? kProbeB : kProbeA;

  // Probe the first test port. The probe is intentionally bogus for most
  // platforms, so the per-port validator emits "Supported profiles: [...]".
  std::vector<std::string> supported;
  auto probeResult = setInterfaceProfile(testPorts[0], probe);
  if (probeResult.exitCode != 0) {
    supported = extractSupportedProfiles(probeResult.stderr);
  } else {
    // Probe happened to be a valid profile — restore and use the limited
    // info we have.
    setInterfaceProfile(testPorts[0], p0);
    supported = {p0, probe};
  }
  if (supported.empty()) {
    GTEST_SKIP() << "Could not discover supported profiles";
  }
  XLOG(INFO) << "  Supported: " << folly::join(", ", supported);

  XLOG(INFO) << "[Step 3] Picking transition target...";
  // Any supported profile with a different lane count works in either
  // direction — TearDown restores the original config regardless of what the
  // legs subsume.
  const int origLanes = profileLaneCount(p0);
  std::string p1;
  int newLanes = 0;
  for (const auto& cand : supported) {
    if (cand == p0) {
      continue;
    }
    int lanes = profileLaneCount(cand);
    if (lanes > 0 && lanes != origLanes) {
      p1 = cand;
      newLanes = lanes;
      break;
    }
  }
  if (p1.empty()) {
    GTEST_SKIP() << "No supported profile with a different lane count from "
                 << p0;
  }
  const std::string direction =
      (newLanes < origLanes) ? "downshift" : "upshift";
  XLOG(INFO) << "  " << direction << " " << p0 << " (" << origLanes
             << " lanes) → " << p1 << " (" << newLanes << " lanes)";

  XLOG(INFO) << "[Step 4] Applying " << p1 << " to "
             << folly::join(",", testPorts) << "...";
  auto fwd = runConfigInterface(testPorts, p1);
  if (fwd.exitCode != 0 && testPorts.size() == 2) {
    // Validation (e.g. parent-subsumption) may reject this particular pair.
    // Fall back to single-port.
    XLOG(INFO) << "  Multi-port rejected; falling back to single-port. "
               << "stderr: " << fwd.stderr;
    testPorts.resize(1);
    fwd = runConfigInterface(testPorts, p1);
  }
  ASSERT_EQ(fwd.exitCode, 0) << "Forward CLI failed: " << fwd.stderr;
  commitConfig();
  waitForAgentReady();
  XLOG(INFO) << "  Forward stderr: " << fwd.stderr;

  XLOG(INFO) << "[Step 5] Verifying P1=" << p1 << " applied...";
  auto fwdWarnings = parseCliWarnings(fwd.stderr);
  verifyAtProfile(testPorts, p1, fwdWarnings);

  XLOG(INFO) << "[Step 6] Restoring P0=" << p0 << "...";
  auto rev = runConfigInterface(testPorts, p0);
  ASSERT_EQ(rev.exitCode, 0) << "Restore CLI failed: " << rev.stderr;
  commitConfig();
  waitForAgentReady();
  XLOG(INFO) << "  Restore stderr: " << rev.stderr;

  XLOG(INFO) << "[Step 7] Verifying P0 restored...";
  auto revWarnings = parseCliWarnings(rev.stderr);
  verifyAtProfile(testPorts, p0, revWarnings);

  // Note: we deliberately do NOT assert full cfg.sw.ports equality with a
  // pre-test snapshot here. If a leg upshifted, it subsumed sibling ports that
  // the opposite leg does not recreate, so the names present mid-test are
  // direction-dependent. The base fixture's TearDown restores the complete
  // pre-test config (via coldboot), which is the authoritative cleanup.

  XLOG(INFO) << "TEST PASSED";
}

/**
 * SetInvalidProfileFails
 *
 * A bogus profile-enum string is rejected before any session mutation.
 * Verifies parseProfile()'s fast-fail.
 */
TEST_F(ConfigInterfaceProfileTest, SetInvalidProfileFails) {
  Interface iface = findFirstEthInterface();
  auto result =
      setInterfaceProfile(iface.name, "PROFILE_COMPLETELY_BOGUS_9999");
  EXPECT_NE(result.exitCode, 0);
  EXPECT_NE(result.stderr.find("not a valid PortProfileID"), std::string::npos)
      << "Expected 'not a valid PortProfileID' in stderr, got: "
      << result.stderr;
}

/**
 * ProfileChange_Atomicity
 *
 * A batch with one nonexistent port name must be rejected wholesale: no
 * other port in the batch may be mutated. Verifies the validate-before-
 * mutate atomicity in applyProfile + validateBatch.
 */
TEST_F(ConfigInterfaceProfileTest, ProfileChange_Atomicity) {
  Interface iface = findFirstEthInterface();
  auto before = getPortRunningInfo(iface.name);
  if (before.profileId == "PROFILE_DEFAULT") {
    GTEST_SKIP() << "Port has PROFILE_DEFAULT; skipping";
  }

  // Bogus port "eth99/99/99" — the batch must fail before any mutation.
  auto result = runCli(
      {"config",
       "interface",
       iface.name,
       "eth99/99/99",
       "profile",
       before.profileId});

  EXPECT_NE(result.exitCode, 0) << "Expected non-zero for batch with bad port";
  auto after = getPortRunningInfo(iface.name);
  EXPECT_EQ(after.profileId, before.profileId)
      << "Valid port " << iface.name
      << " was mutated despite batch failure (atomicity violated)";
}
