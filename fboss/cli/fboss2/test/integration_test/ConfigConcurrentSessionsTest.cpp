// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for concurrent session conflict detection and rebase.
 *
 * Simulates two users by spawning the fboss2-dev binary as a subprocess with
 * a distinct HOME for each call. Session state lives under ~/.fboss2/; the
 * subprocess approach mirrors the original Python test and avoids the
 * in-process-CLI limitation where a single process can only track one active
 * session.
 *
 * The path to the fboss2-dev binary is taken from FBOSS2_DEV_PATH (falling
 * back to "fboss2-dev" in PATH). The test is skipped if no such binary is
 * runnable — when the suite is deployed alongside fboss2-dev on the DUT,
 * the caller sets FBOSS2_DEV_PATH explicitly.
 *
 * Verifies:
 *   1. User1 opens a session and commits successfully.
 *   2. User2's commit is rejected because the system config has changed.
 *   3. User2's rebase surfaces a conflict (same field, different value).
 */

#include <folly/Subprocess.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"
#include "folly/String.h"

using namespace facebook::fboss;
namespace fs = std::filesystem;

namespace {
std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  return s;
}

std::string fboss2DevPath() {
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const char* env = std::getenv("FBOSS2_DEV_PATH");
  return env ? env : "fboss2-dev";
}
} // namespace

class ConfigConcurrentSessionsTest : public Fboss2IntegrationTest {
 protected:
  fs::path user1Home_;
  fs::path user2Home_;

  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    auto tmpBase = fs::temp_directory_path() /
        ("fboss2_concurrent_" + std::to_string(::getpid()));
    user1Home_ = tmpBase / "user1";
    user2Home_ = tmpBase / "user2";
    fs::create_directories(user1Home_);
    fs::create_directories(user2Home_);
  }

  void TearDown() override {
    std::error_code ec;
    fs::remove_all(user1Home_.parent_path(), ec);
    Fboss2IntegrationTest::TearDown();
  }

  struct SubprocResult {
    int exitCode;
    std::string stdout;
    std::string stderr;
  };

  SubprocResult runAsUser(
      const fs::path& home,
      const std::vector<std::string>& args) const {
    std::vector<std::string> cmd = {fboss2DevPath()};
    cmd.insert(cmd.end(), args.begin(), args.end());
    XLOG(INFO) << "[HOME=" << home.string() << "] " << folly::join(" ", cmd);

    folly::Subprocess::Options options;
    options.pipeStdout();
    options.pipeStderr();

    std::vector<std::string> env;
    // Inherit the caller's environment, but override HOME.
    // NOLINTNEXTLINE(concurrency-mt-unsafe): SetUp/TearDown single-threaded
    for (char** e = environ; *e != nullptr; ++e) {
      std::string entry(*e);
      if (entry.rfind("HOME=", 0) != 0) {
        env.push_back(std::move(entry));
      }
    }
    env.push_back("HOME=" + home.string());

    folly::Subprocess proc(cmd, options, /*executable=*/nullptr, &env);
    auto [out, err] = proc.communicate();
    int exitCode = proc.wait().exitStatus();
    return SubprocResult{exitCode, std::move(out), std::move(err)};
  }
};

TEST_F(ConfigConcurrentSessionsTest, ConflictAndRebase) {
  // Quick sanity check — skip if fboss2-dev is not invokable.
  {
    auto probe = runAsUser(user1Home_, {"--help"});
    if (probe.exitCode != 0) {
      GTEST_SKIP() << "Cannot exec fboss2-dev (set FBOSS2_DEV_PATH). stderr="
                   << probe.stderr;
    }
  }

  XLOG(INFO) << "[Step 1] Finding test interface...";
  Interface intf = findFirstEthInterface();
  std::string originalDesc = intf.description;
  XLOG(INFO) << "  Interface " << intf.name << ", original description '"
             << originalDesc << "'";

  XLOG(INFO) << "[Step 2] User1 starts session (description=User1_change)";
  auto r = runAsUser(
      user1Home_,
      {"config", "interface", intf.name, "description", "User1_change"});
  ASSERT_EQ(r.exitCode, 0) << r.stderr;

  XLOG(INFO) << "[Step 3] User2 starts session (description=User2_change)";
  r = runAsUser(
      user2Home_,
      {"config", "interface", intf.name, "description", "User2_change"});
  ASSERT_EQ(r.exitCode, 0) << r.stderr;

  XLOG(INFO) << "[Step 4] User1 commits...";
  r = runAsUser(user1Home_, {"config", "session", "commit"});
  ASSERT_EQ(r.exitCode, 0) << "user1 commit: " << r.stderr;

  auto info = getInterfaceInfo(intf.name);
  EXPECT_EQ(info.description, "User1_change");

  XLOG(INFO) << "[Step 5] User2 attempts commit (should be rejected)...";
  r = runAsUser(user2Home_, {"config", "session", "commit"});
  EXPECT_NE(
      r.stderr.find("system configuration has changed"), std::string::npos)
      << "Expected conflict message; got stderr=" << r.stderr
      << " stdout=" << r.stdout;

  XLOG(INFO) << "[Step 6] User2 attempts rebase...";
  r = runAsUser(user2Home_, {"config", "session", "rebase"});
  auto combined = toLower(r.stdout) + toLower(r.stderr);
  EXPECT_NE(combined.find("conflict"), std::string::npos)
      << "Expected 'conflict' in rebase output; got stdout=" << r.stdout
      << " stderr=" << r.stderr;

  XLOG(INFO) << "[Cleanup] Restoring original description '" << originalDesc
             << "'";
  auto restore =
      runCli({"config", "interface", intf.name, "description", originalDesc});
  if (restore.exitCode == 0) {
    commitConfig();
  } else {
    XLOG(WARN) << "  Restore CLI failed: " << restore.stderr;
  }

  XLOG(INFO) << "TEST PASSED";
}
