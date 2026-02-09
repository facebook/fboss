// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/json/dynamic.h>
#include <gtest/gtest.h>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace facebook::fboss {

/**
 * Represents the result of a CLI command execution.
 */
struct CliResult {
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
  int mtu;
  std::vector<std::string> addresses;
  std::string description;
};

/**
 * CliTest is the base class for CLI end-to-end tests.
 *
 * Unlike the link tests, CLI tests don't need to be concerned with the SAI
 * or what SAI we're using, since we're testing the CLI and the CLI is
 * largely platform independent.
 *
 * The tests run against a live FBOSS agent and execute actual CLI commands
 * by directly invoking the CLI library code (not spawning a subprocess).
 */
class CliTest : public ::testing::Test {
 public:
  ~CliTest() override = default;

 protected:
  void SetUp() override;
  void TearDown() override;

  /**
   * Run a CLI command with the given arguments.
   * The --fmt json flag is automatically prepended.
   * @param args The command arguments (e.g., {"show", "interface"})
   * @return CliResult with exit code, stdout, and stderr
   */
  CliResult runCli(const std::vector<std::string>& args) const;

  /**
   * Run a CLI command and parse the JSON output.
   * Throws if the command fails or output is not valid JSON.
   * @param args The command arguments
   * @return Parsed JSON as folly::dynamic
   */
  folly::dynamic runCliJson(const std::vector<std::string>& args) const;

  /**
   * Run a shell command and return the result.
   * @param args Command and arguments
   * @return CliResult with exit code, stdout, and stderr
   */
  CliResult runCmd(const std::vector<std::string>& args) const;

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
   * Find the first suitable ethernet interface for testing.
   * Only returns ethernet interfaces (starting with 'eth') with a valid VLAN.
   * @return Interface object
   */
  Interface findFirstEthInterface() const;

  /**
   * Commit the current configuration session.
   */
  void commitConfig() const;

  /**
   * Get the MTU of a kernel interface using 'ip -json link'.
   * @param vlanId The VLAN ID (interface name will be fboss<vlanId>)
   * @return MTU value, or -1 if interface not found
   */
  int getKernelInterfaceMtu(int vlanId) const;

  /**
   * Discard any pending config session by deleting session files.
   * This ensures each test starts with a fresh session based on current HEAD.
   */
  void discardSession() const;

 private:
  Interface parseInterfaceJson(const folly::dynamic& data) const;

  /**
   * Execute a CLI command by parsing args and running the command.
   * Captures stdout/stderr and returns the result.
   */
  CliResult executeCliCommand(const std::vector<std::string>& args) const;
};

/**
 * Main function for CLI tests.
 * Initializes gtest and runs all CLI tests.
 */
int cliTestMain(int argc, char** argv);

} // namespace facebook::fboss
