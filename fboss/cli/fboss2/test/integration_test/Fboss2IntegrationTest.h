// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/json/dynamic.h>
#include <gtest/gtest.h>
#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace facebook::fboss {

/**
 * Fboss2IntegrationTest is the base class for CLI end-to-end tests.
 *
 * Unlike the link tests, CLI tests don't need to be concerned with the SAI
 * or what SAI we're using, since we're testing the CLI and the CLI is
 * largely platform independent.
 *
 * The tests run against a live FBOSS agent and execute actual CLI commands
 * by directly invoking the CLI library code (not spawning a subprocess).
 */
class Fboss2IntegrationTest : public ::testing::Test {
 public:
  /**
   * Represents the result of a CLI command execution.
   */
  struct Result {
    int exitCode;
    std::string stdout;
    std::string stderr;
  };

  /**
   * Represents a network interface from the output of 'show interface'.
   */
  struct Interface {
    std::string name;
    std::string status;
    std::string speed;
    std::optional<int> vlan;
    int mtu{};
    std::vector<std::string> addresses;
    std::string description;
  };

  ~Fboss2IntegrationTest() override = default;

 protected:
  void SetUp() override;
  void TearDown() override;

  /**
   * Run a CLI command with the given arguments.
   * The --fmt json flag is automatically prepended.
   * @param args The command arguments (e.g., {"show", "interface"})
   * @return Result with exit code, stdout, and stderr
   */
  Result runCli(const std::vector<std::string>& args) const;

  /**
   * Run a CLI command and parse the JSON output.
   * Throws if the command fails or output is not valid JSON.
   * @param args The command arguments
   * @return Parsed JSON as folly::dynamic
   */
  folly::dynamic runCliJson(const std::vector<std::string>& args) const;

  /**
   * Fetch the agent's running config via thrift and return it parsed.
   */
  folly::dynamic getRunningConfig() const;

  /**
   * Find a (vlanId, portName) pair where vlanId appears in sw.vlans and the
   * port is a member of that VLAN via sw.vlanPorts. Returns nullopt on
   * switches configured in a port-based / L3-routed style with no L2 VLAN
   * memberships — tests that need L2-VLAN commands (e.g. 'config vlan <id>
   * port <name> taggingMode', 'config vlan <id> static-mac ...') should
   * GTEST_SKIP when this returns nullopt.
   */
  std::optional<std::pair<int, std::string>> findConfiguredVlanPort() const;

  /**
   * Run a shell command and return the result.
   * @param args Command and arguments
   * @return Result with exit code, stdout, and stderr
   */
  Result runCmd(const std::vector<std::string>& args) const;

  /**
   * Get information about a specific interface.
   * @param interfaceName The interface name (e.g., "eth1/1/1")
   * @return Interface object with the interface details
   */
  Interface getInterfaceInfo(const std::string& interfaceName) const;

  /**
   * Get all interfaces from the system.
   * @return Map of interface name to Interface object
   */
  std::map<std::string, Interface> getAllInterfaces() const;

  /**
   * Pick an ethernet interface for testing.
   *
   * Interfaces whose status indicates they are up are strongly preferred — if
   * at least one matches, a random one is returned. If none are up, a random
   * interface from all ethernet candidates is returned. An "ethernet
   * candidate" is any interface whose name starts with "eth" and has
   * VLAN > 1. Throws if no candidates exist.
   *
   * The selection is randomized (thread-local mt19937) to reduce the chance
   * of piling test load onto the same port across back-to-back runs.
   */
  Interface findFirstEthInterface() const;

  /**
   * Commit the current configuration session.
   */
  void commitConfig() const;

  /**
   * Get the MTU of a kernel interface using 'ip -json link'.
   * @param vlanId The VLAN ID (interface name will be fboss<vlanId>)
   * @return MTU value
   * @throws std::runtime_error if interface not found
   */
  int getKernelInterfaceMtu(int vlanId) const;

  /**
   * Running port info returned from a direct Thrift call to the agent.
   */
  struct PortRunningInfo {
    int64_t speedMbps{0};
    std::string profileId;
  };

  /**
   * Poll getInterfaceInfo() until condition(info) is true or timeout expires.
   * Returns the last observed Interface so callers can assert on the value.
   */
  Interface waitForInterfaceInfo(
      const std::string& interfaceName,
      const std::function<bool(const Interface&)>& condition,
      std::chrono::seconds timeout = std::chrono::seconds(30),
      std::chrono::seconds interval = std::chrono::seconds(2)) const;

  /**
   * Poll getKernelInterfaceMtu() until condition(mtu) is true or timeout
   * expires. Returns the last observed MTU value.
   */
  int waitForKernelMtu(
      int vlanId,
      const std::function<bool(int)>& condition,
      std::chrono::seconds timeout = std::chrono::seconds(30),
      std::chrono::seconds interval = std::chrono::seconds(2)) const;

  /**
   * Query the agent directly via getAllPortInfo() and return the running
   * speed and profileID for the named port.
   * @param portName The port name (e.g., "eth1/1/1")
   * @throws std::runtime_error if the port is not found
   */
  PortRunningInfo getPortRunningInfo(const std::string& portName) const;

  /**
   * Poll getPortRunningInfo() until condition(info) returns true or timeout
   * expires. Returns the last observed PortRunningInfo so callers can assert
   * on the final value even when the condition is not met.
   * @param portName  The port name (e.g., "eth1/1/1")
   * @param condition Predicate evaluated on each poll result
   * @param timeout   Max time to wait (default: 30 s)
   * @param interval  Sleep between polls (default: 2 s)
   */
  PortRunningInfo waitForPortRunningInfo(
      const std::string& portName,
      const std::function<bool(const PortRunningInfo&)>& condition,
      std::chrono::seconds timeout = std::chrono::seconds(30),
      std::chrono::seconds interval = std::chrono::seconds(2)) const;

  /**
   * Discard any pending config session by deleting session files.
   * This ensures each test starts with a fresh session based on current HEAD.
   */
  void discardSession() const;

  /**
   * Wait until the FBOSS agent is responsive (ready to serve thrift requests).
   * Polls 'show interface' until it succeeds or timeout is reached.
   * Use this after triggering a warmboot or coldboot restart.
   * @param timeout Maximum time to wait (default: 120 seconds)
   */
  void waitForAgentReady(
      std::chrono::seconds timeout = std::chrono::seconds(300)) const;

 private:
  Interface parseInterfaceJson(const folly::dynamic& data) const;

  /**
   * Execute a CLI command by parsing args and running the command.
   * Captures stdout/stderr and returns the result.
   */
  Result executeCliCommand(const std::vector<std::string>& args) const;
};
} // namespace facebook::fboss
