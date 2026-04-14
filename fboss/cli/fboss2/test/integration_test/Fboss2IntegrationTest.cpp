// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

#include <CLI/App.hpp>
#include <CLI/Error.hpp>
#include <fmt/format.h>
#include <folly/String.h>
#include <folly/Subprocess.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/Init.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "fboss/cli/fboss2/utils/CmdInitUtils.h"

namespace fs = std::filesystem;

namespace facebook::fboss {

void Fboss2IntegrationTest::SetUp() {
  XLOG(INFO) << "Fboss2IntegrationTest::SetUp - starting CLI test";
  // Discard any stale session from previous runs to ensure we start fresh
  discardSession();
}

void Fboss2IntegrationTest::TearDown() {
  XLOG(INFO) << "Fboss2IntegrationTest::TearDown - cleaning up CLI test";
}

void Fboss2IntegrationTest::discardSession() const {
  // Delete the session files to ensure we start with a fresh session
  // based on the current HEAD. ConfigSession::initializeSession() will
  // reset internal state when it detects no session file exists.
  // NOLINTNEXTLINE(concurrency-mt-unsafe): HOME is read-only in practice
  const char* home = std::getenv("HOME");
  if (home == nullptr) {
    XLOG(WARN) << "HOME environment variable not set, cannot discard session";
    return;
  }
  fs::path sessionDir = fs::path(home) / ".fboss2";
  fs::path sessionConfig = sessionDir / "agent.conf";
  fs::path sessionMetadata = sessionDir / "cli_metadata.json";

  std::error_code ec;
  if (fs::exists(sessionConfig, ec)) {
    XLOG(INFO) << "Discarding session config: " << sessionConfig.string();
    fs::remove(sessionConfig, ec);
    if (ec) {
      XLOG(WARN) << "Failed to remove session config: " << ec.message();
    }
  }
  if (fs::exists(sessionMetadata, ec)) {
    XLOG(INFO) << "Discarding session metadata: " << sessionMetadata.string();
    fs::remove(sessionMetadata, ec);
    if (ec) {
      XLOG(WARN) << "Failed to remove session metadata: " << ec.message();
    }
  }
}

Fboss2IntegrationTest::Result Fboss2IntegrationTest::executeCliCommand(
    const std::vector<std::string>& args) const {
  // Create a new CLI::App for this command
  CLI::App app{"FBOSS CLI Test"};
  utils::initApp(app);

  // Build argv-style argument list
  // Prepend program name and --fmt json
  std::vector<std::string> fullArgs = {
      "fboss2_integration_test", "--fmt", "json"};
  fullArgs.insert(fullArgs.end(), args.begin(), args.end());

  // Convert to argc/argv format
  std::vector<char*> argv;
  argv.reserve(fullArgs.size());
  for (auto& arg : fullArgs) {
    argv.push_back(arg.data());
  }

  // Redirect stdout and stderr to capture output
  std::stringstream capturedStdout;
  std::stringstream capturedStderr;
  std::streambuf* oldStdout = std::cout.rdbuf(capturedStdout.rdbuf());
  std::streambuf* oldStderr = std::cerr.rdbuf(capturedStderr.rdbuf());

  int exitCode = 0;
  try {
    // Parse and execute the command
    app.parse(static_cast<int>(argv.size()), argv.data());
  } catch (const CLI::ParseError& e) {
    exitCode = app.exit(e);
  } catch (const std::exception& e) {
    capturedStderr << "Error: " << e.what() << "\n";
    exitCode = 1;
  }

  // Restore original stdout/stderr
  std::cout.rdbuf(oldStdout);
  std::cerr.rdbuf(oldStderr);

  return Result{exitCode, capturedStdout.str(), capturedStderr.str()};
}

Fboss2IntegrationTest::Result Fboss2IntegrationTest::runCli(
    const std::vector<std::string>& args) const {
  XLOG(INFO) << "Running CLI command: " << folly::join(" ", args);
  return executeCliCommand(args);
}

folly::dynamic Fboss2IntegrationTest::runCliJson(
    const std::vector<std::string>& args) const {
  auto result = runCli(args);
  EXPECT_EQ(result.exitCode, 0) << "CLI command failed: " << result.stderr;

  if (result.exitCode != 0 || result.stdout.empty()) {
    return folly::dynamic::object();
  }
  return folly::parseJson(result.stdout);
}

Fboss2IntegrationTest::Result Fboss2IntegrationTest::runCmd(
    const std::vector<std::string>& args) const {
  XLOG(INFO) << "Running command: " << folly::join(" ", args);

  folly::Subprocess::Options options;
  options.pipeStdout();
  options.pipeStderr();

  folly::Subprocess proc(args, options);
  auto [stdout, stderr] = proc.communicate();
  auto exitStatus = proc.wait().exitStatus();

  return Result{exitStatus, stdout, stderr};
}

Fboss2IntegrationTest::Interface Fboss2IntegrationTest::parseInterfaceJson(
    const folly::dynamic& data) const {
  Fboss2IntegrationTest::Interface intf;
  intf.name = data["name"].asString();
  intf.status = data["status"].asString();
  intf.speed = data["speed"].asString();
  if (data.count("vlan") && !data["vlan"].isNull()) {
    intf.vlan = static_cast<int>(data["vlan"].asInt());
  }
  intf.mtu = static_cast<int>(data["mtu"].asInt());

  // Parse prefixes
  if (data.count("prefixes")) {
    for (const auto& prefix : data["prefixes"]) {
      intf.addresses.push_back(
          fmt::format(
              "{}/{}",
              prefix["ip"].asString(),
              prefix["prefixLength"].asInt()));
    }
  }

  intf.description = data.getDefault("description", "").asString();
  return intf;
}

std::map<std::string, Fboss2IntegrationTest::Interface>
Fboss2IntegrationTest::getAllInterfaces() const {
  auto json = runCliJson({"show", "interface"});
  std::map<std::string, Fboss2IntegrationTest::Interface> interfaces;

  // JSON has a host key containing the interfaces
  for (const auto& [host, hostData] : json.items()) {
    if (!hostData.isObject() || !hostData.count("interfaces")) {
      continue;
    }
    for (const auto& intfData : hostData["interfaces"]) {
      auto intf = parseInterfaceJson(intfData);
      interfaces[intf.name] = intf;
    }
  }

  return interfaces;
}

Fboss2IntegrationTest::Interface Fboss2IntegrationTest::getInterfaceInfo(
    const std::string& interfaceName) const {
  auto json = runCliJson({"show", "interface", interfaceName});

  XLOG(DBG2) << "getInterfaceInfo JSON: " << folly::toPrettyJson(json);

  for (const auto& [host, hostData] : json.items()) {
    XLOG(DBG2) << "  Host: " << host << ", isObject: " << hostData.isObject()
               << ", hasInterfaces: " << hostData.count("interfaces");
    if (!hostData.isObject() || !hostData.count("interfaces")) {
      continue;
    }
    for (const auto& intfData : hostData["interfaces"]) {
      XLOG(DBG2) << "    Interface: " << intfData["name"].asString();
      if (intfData["name"].asString() == interfaceName) {
        return parseInterfaceJson(intfData);
      }
    }
  }

  throw std::runtime_error(
      fmt::format("Interface {} not found", interfaceName));
}

Fboss2IntegrationTest::Interface Fboss2IntegrationTest::findFirstEthInterface()
    const {
  auto interfaces = getAllInterfaces();

  for (const auto& [name, intf] : interfaces) {
    if (name.rfind("eth", 0) == 0 && intf.vlan.has_value() && *intf.vlan > 1) {
      return intf;
    }
  }

  throw std::runtime_error(
      "No suitable ethernet interface found with VLAN > 1");
}

void Fboss2IntegrationTest::commitConfig() const {
  auto result = runCli({"config", "session", "commit"});
  ASSERT_EQ(result.exitCode, 0) << "Failed to commit config: " << result.stderr;
}

void Fboss2IntegrationTest::waitForAgentReady(
    std::chrono::seconds timeout) const {
  XLOG(INFO) << "Waiting for agent to become ready (timeout: "
             << timeout.count() << "s)...";
  // Wait 5s before the first poll to give the agent time to shut down and
  // start coming back up. Polling immediately after a restart can interfere
  // with the initialization sequence.
  XLOG(INFO) << "  Waiting 5s before first poll...";
  // NOLINTNEXTLINE(facebook-hte-BadCall-sleep_for)
  std::this_thread::sleep_for(std::chrono::seconds(5));
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    auto result = executeCliCommand({"show", "interface"});
    if (result.exitCode == 0) {
      XLOG(INFO) << "Agent is ready";
      return;
    }
    // Two expected failure modes during restart:
    // 1. "Connection refused"  — process not yet listening
    // 2. "switch is still initializing" — process up, HW init in progress
    // Both resolve naturally once the agent finishes warmboot/coldboot (~50s).
    XLOG(DBG1) << "Agent not ready: " << result.stderr;
    XLOG(INFO) << "Agent not ready yet, retrying in 10s...";
    // NOLINTNEXTLINE(facebook-hte-BadCall-sleep_for)
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
  FAIL() << "Agent did not become ready within " << timeout.count()
         << " seconds";
}

int Fboss2IntegrationTest::getKernelInterfaceMtu(int vlanId) const {
  auto kernelIntf = fmt::format("fboss{}", vlanId);
  // Use full path to ip command since folly::Subprocess doesn't search PATH
  auto result = runCmd({"/usr/sbin/ip", "-json", "link", "show", kernelIntf});

  if (result.exitCode != 0) {
    throw std::runtime_error(
        fmt::format(
            "Kernel interface {} not found ('ip link show' returned {})",
            kernelIntf,
            result.exitCode));
  }

  auto json = folly::parseJson(result.stdout);
  if (json.empty()) {
    throw std::runtime_error(
        fmt::format("No data returned for kernel interface {}", kernelIntf));
  }

  return static_cast<int>(json[0]["mtu"].asInt());
}

} // namespace facebook::fboss

#ifdef IS_OSS
FOLLY_INIT_LOGGING_CONFIG("DBG2; default:async=true");
#else
FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");
#endif

int main(int argc, char* argv[]) {
  // Initialize gtest first so it consumes --gtest_* flags before folly::init
  ::testing::InitGoogleTest(&argc, argv);

  // Initialize folly singletons before using CLI components
  folly::Init init(&argc, &argv, /* removeFlags = */ false);

  return RUN_ALL_TESTS();
}
