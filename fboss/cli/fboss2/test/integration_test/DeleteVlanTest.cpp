// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for `fboss2-dev delete vlan <id>`.
 *
 * This exercises the CLI wiring, live running-config read, and session
 * round-trip against a real agent. The fine-grained mutation semantics
 * (barebone-interface + static-MAC cascade removal) and the referenced-VLAN
 * refusals (default VLAN, port membership, ingress VLAN, SVI with IPs) are
 * covered by the CmdDeleteVlan unit tests against a controlled seed config.
 *
 * CreateThenDeleteVlan always creates a fresh VLAN in the config session
 * (`config vlan <id> static-mac add` — there is no bare `config vlan <id>`
 * create verb; the static MAC it adds is cascade-removed by the delete),
 * deletes it in the same session, and discards. It runs unconditionally on
 * any DUT config and never commits, so it is safe on any agent state.
 *
 * CommitDeleteExistingVlan covers the commit path: it deletes a committed,
 * unreferenced VLAN from the running config, commits, and waits for the VLAN
 * to disappear from the running config. waitForRunningConfig() tolerates the
 * agent reload a commit can trigger. It skips when the running config has no
 * deletable VLAN. It does not create its own candidate first: committing a
 * config that adds a fresh VLAN together with a static MAC crashes the
 * hw_agent (SIGSEGV in SaiFdbManager::addFdbEntry — the FDB entry is
 * programmed before the VLAN's router interface exists in SAI), and a staged
 * VLAN without content nets to an empty session diff, so there is no
 * commit-safe way to recreate the consumed VLAN either. Testbed
 * re-preparation restores it.
 *
 * Requirements:
 *   - FBOSS agent is running with a valid configuration
 *   - Test is run as root (or with sudo) on a DUT
 */

#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <set>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;

class DeleteVlanTest : public Fboss2IntegrationTest {
 protected:
  // First VLAN ID in [2, 4094] that is neither configured nor referenced
  // anywhere in the running config, for tests that must create a fresh VLAN.
  int pickUnusedVlanId() {
    auto config = getRunningConfig();
    std::set<int> used;
    if (config.isObject() && config.count("sw")) {
      const auto& sw = config["sw"];
      if (sw.count("defaultVlan")) {
        used.insert(sw["defaultVlan"].asInt());
      }
      if (sw.count("ports")) {
        for (const auto& p : sw["ports"]) {
          if (p.count("ingressVlan")) {
            used.insert(p["ingressVlan"].asInt());
          }
        }
      }
      if (sw.count("vlanPorts")) {
        for (const auto& vp : sw["vlanPorts"]) {
          if (vp.count("vlanID")) {
            used.insert(vp["vlanID"].asInt());
          }
        }
      }
      if (sw.count("interfaces")) {
        for (const auto& i : sw["interfaces"]) {
          if (i.count("vlanID")) {
            used.insert(i["vlanID"].asInt());
          }
        }
      }
      if (sw.count("vlans")) {
        for (const auto& v : sw["vlans"]) {
          if (v.count("id")) {
            used.insert(v["id"].asInt());
          }
        }
      }
    }
    for (int id = 2; id <= 4094; ++id) {
      if (!used.count(id)) {
        return id;
      }
    }
    return 0;
  }

  // A committed VLAN that `delete vlan` will accept: not the default VLAN,
  // no port uses it as ingress VLAN, no port is a member of it, and its L3
  // interface (if any) has no IP addresses. Returns 0 if none exists.
  int pickDeletableVlanId() {
    auto config = getRunningConfig();
    if (!config.isObject() || !config.count("sw")) {
      return 0;
    }
    const auto& sw = config["sw"];
    std::set<int> referenced;
    if (sw.count("defaultVlan")) {
      referenced.insert(sw["defaultVlan"].asInt());
    }
    if (sw.count("ports")) {
      for (const auto& p : sw["ports"]) {
        if (p.count("ingressVlan")) {
          referenced.insert(p["ingressVlan"].asInt());
        }
      }
    }
    if (sw.count("vlanPorts")) {
      for (const auto& vp : sw["vlanPorts"]) {
        if (vp.count("vlanID")) {
          referenced.insert(vp["vlanID"].asInt());
        }
      }
    }
    if (sw.count("interfaces")) {
      for (const auto& i : sw["interfaces"]) {
        if (i.count("vlanID") && i.count("ipAddresses") &&
            !i["ipAddresses"].empty()) {
          referenced.insert(i["vlanID"].asInt());
        }
      }
    }
    if (sw.count("vlans")) {
      for (const auto& v : sw["vlans"]) {
        if (v.count("id") && !referenced.count(v["id"].asInt())) {
          return v["id"].asInt();
        }
      }
    }
    return 0;
  }

  // Whether the running config has a VLAN with the given id.
  static bool hasVlan(const folly::dynamic& config, int vlanId) {
    if (!config.isObject() || !config.count("sw") ||
        !config["sw"].count("vlans")) {
      return false;
    }
    for (const auto& v : config["sw"]["vlans"]) {
      if (v.count("id") && v["id"].asInt() == vlanId) {
        return true;
      }
    }
    return false;
  }

  // Name of any port in the running config (static-mac add needs one as the
  // egress port). std::nullopt if the config has no named ports.
  std::optional<std::string> anyPortName() {
    auto config = getRunningConfig();
    if (config.isObject() && config.count("sw") &&
        config["sw"].count("ports")) {
      for (const auto& p : config["sw"]["ports"]) {
        if (p.count("name")) {
          return p["name"].asString();
        }
      }
    }
    return std::nullopt;
  }
};

TEST_F(DeleteVlanTest, CreateThenDeleteVlan) {
  const int vlanId = pickUnusedVlanId();
  ASSERT_NE(vlanId, 0) << "no free VLAN ID in [2, 4094]";
  auto port = anyPortName();
  ASSERT_TRUE(port.has_value()) << "running config has no named ports";
  const std::string id = std::to_string(vlanId);

  XLOG(INFO) << "[Step 1] Creating VLAN " << id << " via static-mac add";
  auto create = runCli(
      {"config", "vlan", id, "static-mac", "add", "02:00:00:00:0F:F5", *port});
  if (create.exitCode != 0) {
    discardSession();
  }
  ASSERT_EQ(create.exitCode, 0)
      << "stdout=" << create.stdout << " stderr=" << create.stderr;
  EXPECT_THAT(create.stdout, HasSubstr("Created VLAN"));

  XLOG(INFO) << "[Step 2] Deleting VLAN " << id << " in the same session";
  auto result = runCli({"delete", "vlan", id});
  // Discard before asserting so a failure cannot leave the session dirty;
  // never commit a freshly created VLAN (see file header).
  discardSession();
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr(id));

  XLOG(INFO) << "[Step 3] Verifying deleting it again reports non-existence";
  auto again = runCli({"delete", "vlan", id});
  discardSession();
  EXPECT_NE(again.exitCode, 0)
      << "second delete of VLAN " << id << " should fail: it no longer exists";
}

TEST_F(DeleteVlanTest, CommitDeleteExistingVlan) {
  const int vlanId = pickDeletableVlanId();
  if (vlanId == 0) {
    GTEST_SKIP() << "running config has no committed, unreferenced VLAN";
  }
  const std::string id = std::to_string(vlanId);

  XLOG(INFO) << "[Step 1] Deleting existing VLAN " << id << " and committing";
  auto result = runCli({"delete", "vlan", id});
  if (result.exitCode != 0) {
    discardSession();
  }
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 2] Verifying VLAN " << id
             << " is gone from the running config";
  auto config = waitForRunningConfig(
      [&](const folly::dynamic& c) { return !hasVlan(c, vlanId); });
  EXPECT_FALSE(hasVlan(config, vlanId))
      << "VLAN " << id << " still in running config after committed delete";

  XLOG(INFO) << "[Step 3] Verifying deleting it again reports non-existence";
  auto again = runCli({"delete", "vlan", id});
  discardSession();
  EXPECT_NE(again.exitCode, 0)
      << "second delete of VLAN " << id << " should fail: it no longer exists";
}
