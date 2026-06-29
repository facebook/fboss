// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

/**
 * Shared fixture + helpers for BGP config integration tests.
 *
 * BGP config is not pushed into the agent's running config like the
 * SwitchConfig-backed commands. Instead the CLI stages a BGP++
 * thrift-compatible JSON file at ~/.fboss2/bgp_config.json; `config session
 * commit` promotes it to /etc/coop/bgpcpp/bgpcpp.conf and restarts the bgp_pp
 * daemon. These helpers let the global-attribute tests and the
 * session-lifecycle tests share the staging, commit, daemon-state, and
 * system-config plumbing.
 */

#include <folly/FileUtil.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <thread>

#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

namespace facebook::fboss {

class ConfigBgpTestBase : public Fboss2IntegrationTest {
 protected:
  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    if (bgpDaemonActiveState() != "active") {
      GTEST_SKIP() << "skipping because bgp is not active";
    }
  }

  // Staged session file written by the CLI (~/.fboss2/bgp_config.json).
  std::string bgpSessionPath() const {
    // NOLINTNEXTLINE(concurrency-mt-unsafe): HOME is read-only in practice
    const char* home = std::getenv("HOME");
    if (home == nullptr) {
      throw std::runtime_error("HOME environment variable not set");
    }
    return std::string(home) + "/.fboss2/bgp_config.json";
  }

  // System config consumed by the bgp_pp daemon (written on commit).
  std::string systemBgpConfigPath() const {
    return "/etc/coop/bgpcpp/bgpcpp.conf";
  }

  // Remove any pre-existing staged BGP session so each test starts clean.
  void clearBgpSession() const {
    std::remove(bgpSessionPath().c_str());
  }

  folly::dynamic readBgpSessionConfig() const {
    std::string content;
    if (!folly::readFile(bgpSessionPath().c_str(), content)) {
      throw std::runtime_error(
          "Failed to read BGP session config at " + bgpSessionPath());
    }
    return folly::parseJson(content);
  }

  folly::dynamic readSystemBgpConfig() const {
    std::string content;
    if (!folly::readFile(systemBgpConfigPath().c_str(), content)) {
      throw std::runtime_error(
          "Failed to read system BGP config at " + systemBgpConfigPath());
    }
    return folly::parseJson(content);
  }

  // Root of the git repo that versions both the agent config (cli/agent.conf)
  // and the BGP config (bgpcpp/bgpcpp.conf).
  std::string coopRepoDir() const {
    return "/etc/coop";
  }

  // Current git HEAD sha of the /etc/coop repo (trimmed; "" on failure).
  // runCmd uses folly::Subprocess, which does not search PATH, so git is
  // invoked by full path.
  std::string gitHead() const {
    auto r = runCmd({"/usr/bin/git", "-C", coopRepoDir(), "rev-parse", "HEAD"});
    std::string s = r.stdout;
    while (!s.empty() &&
           (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
      s.pop_back();
    }
    return s;
  }

  // True if bgpcpp/bgpcpp.conf is tracked (present in the tree) at the given
  // revision. `git cat-file -e <sha>:<path>` exits 0 iff the blob exists.
  bool bgpTrackedAtRevision(const std::string& sha) const {
    auto r = runCmd(
        {"/usr/bin/git",
         "-C",
         coopRepoDir(),
         "cat-file",
         "-e",
         sha + ":bgpcpp/bgpcpp.conf"});
    return r.exitCode == 0;
  }

  // Stage a `config protocol bgp global <attr> <value>` change (no commit).
  // Returns the staged session JSON.
  folly::dynamic runBgpGlobal(const std::string& attr, const std::string& value)
      const {
    clearBgpSession();
    XLOG(INFO) << "Running: config protocol bgp global " << attr << " "
               << value;
    auto result = runCli({"config", "protocol", "bgp", "global", attr, value});
    EXPECT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    // Handler error strings are printed to stdout with exit code 0, so guard
    // against silent failures by rejecting an "Error" prefix.
    EXPECT_THAT(result.stdout, ::testing::Not(::testing::HasSubstr("Error:")));
    return readBgpSessionConfig();
  }

  // Reset bgp_pp's systemd start-limit burst counter. Tests commit several
  // times in quick succession, and each commit restarts bgp_pp; without this,
  // systemd's default StartLimitBurst (5 starts / 10s) refuses a rapid restart
  // with "start limit hit" — a test artifact, not a product issue.
  void resetBgpDaemonLimit() const {
    runCmd({"/usr/bin/systemctl", "reset-failed", "bgp_pp"});
  }

  // Run `config session commit` and return the short SHA parsed from
  // "Config session committed successfully as <sha>". Empty string on failure.
  std::string commitAndGetSha() const {
    resetBgpDaemonLimit();
    auto r = runCli({"config", "session", "commit"});
    EXPECT_EQ(r.exitCode, 0) << "stderr=" << r.stderr;
    const std::string marker = "as ";
    auto pos = r.stdout.find(marker);
    if (pos == std::string::npos) {
      return "";
    }
    pos += marker.size();
    auto end = r.stdout.find_first_of(" .\n", pos);
    return r.stdout.substr(pos, end - pos);
  }

  // Stage a global change, commit it, wait for bgp_pp to come back, and return
  // the promoted system config (the authoritative post-commit location).
  // A no-op commit (staged value already in the system config) yields no SHA
  // and is fine: the caller's assertions on the returned config still verify
  // the value; commit/restart mechanics are covered by ConfigBgpSessionTest.
  folly::dynamic setAndCommit(const std::string& attr, const std::string& value)
      const {
    discardSession();
    runBgpGlobal(attr, value);
    commitAndGetSha();
    EXPECT_TRUE(waitForBgpDaemonActive())
        << "bgp_pp did not return active after commit; state="
        << bgpDaemonActiveState();
    return readSystemBgpConfig();
  }

  // Trailing-whitespace-trimmed stdout of a `systemctl` query for bgp_pp.
  std::string systemctlValue(const std::string& property) const {
    // Full path: folly::Subprocess (used by runCmd) does not search PATH.
    auto r = runCmd(
        {"/usr/bin/systemctl", "show", "bgp_pp", "-p", property, "--value"});
    std::string s = r.stdout;
    while (!s.empty() &&
           (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
      s.pop_back();
    }
    return s;
  }

  // "active" / "inactive" / "failed" / ... (trimmed).
  std::string bgpDaemonActiveState() const {
    auto r = runCmd({"/usr/bin/systemctl", "is-active", "bgp_pp"});
    std::string s = r.stdout;
    while (!s.empty() &&
           (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
      s.pop_back();
    }
    return s;
  }

  // bgp_pp main PID per systemd ("0" if not running).
  std::string bgpDaemonMainPid() const {
    return systemctlValue("MainPID");
  }

  // Poll until bgp_pp reports active or the timeout elapses.
  bool waitForBgpDaemonActive(
      std::chrono::seconds timeout = std::chrono::seconds(60)) const {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
      if (bgpDaemonActiveState() == "active") {
        return true;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return bgpDaemonActiveState() == "active";
  }
};

} // namespace facebook::fboss
