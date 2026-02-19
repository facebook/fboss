/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss::utils {
class PortMap;
}

namespace facebook::fboss {

/**
 * ConfigSession manages configuration editing sessions for the fboss2 CLI.
 *
 * OVERVIEW:
 * ConfigSession provides a session-based workflow for editing FBOSS agent
 * configuration. It maintains one or more session files that can be edited
 * and then atomically committed to the system configuration.
 *
 * SINGLETON PATTERN:
 * ConfigSession is typically accessed via getInstance(), which currently
 * returns a per-UNIX-user singleton. The singleton is created on first
 * access and persists across CLI commands until commit() is called.
 *
 * TYPICAL USAGE:
 * Most CLI commands follow this pattern:
 *   1. Get the singleton: auto& session = ConfigSession::getInstance();
 *   2. Access the config: auto& config = session.getAgentConfig();
 *   3. Modify the config: config.sw()->ports()[0].name() = "eth0";
 *   4. Save changes: session.saveConfig();
 *
 * The changes are saved to the session file (currently ~/.fboss2/agent.conf)
 * but are NOT applied to the running system until explicitly committed.
 *
 * COMMIT FLOW:
 * To apply session changes to the system:
 *   1. User runs: fboss2 config session commit
 *   2. ConfigSession::commit() is called, which:
 *      a. Determines the next revision number (e.g., r5)
 *      b. Atomically writes session config to /etc/coop/cli/agent-r5.conf
 *      c. Atomically updates the /etc/coop/agent.conf symlink to point to
 *         agent-r5.conf and calls reloadConfig() on the wedge_agent to
 *         reload its configuration
 *   3. The session file is cleared (ready for next edit session)
 *
 * ROLLBACK FLOW:
 * To revert to a previous configuration:
 *   1. User runs: fboss2 config rollback [rN]
 *   2. ConfigSession::rollback() is called, which:
 *      a. Identifies the target revision (previous or specified)
 *      b. Atomically updates /etc/coop/agent.conf symlink to point to
 * agent-rN.conf c. Calls wedge_agent to reload the configuration
 *
 * CONFIGURATION FILES:
 * - Session file: ~/.fboss2/agent.conf (per-user, temporary edits)
 * - System config: <config_dir>/agent.conf (symlink to current revision)
 * - Revision files: <config_dir>/cli/agent-rN.conf (committed configs)
 *
 * Where <config_dir> is determined by AgentDirectoryUtil::getConfigDirectory()
 * (typically /etc/coop, derived from the parent of the config directory path).
 *
 * THREAD SAFETY:
 * ConfigSession is NOT thread-safe. It is designed for single-threaded CLI
 * command execution. The code is safe in face of concurrent usage from
 * multiple processes, e.g. two users trying to commit a config at the same
 * time should not lead to a partially committed config or any process being
 * able to read a partially written file.
 */
class ConfigSession {
 public:
  ConfigSession();
  virtual ~ConfigSession() = default;

  // Get or create the current config session
  // If no session exists, copies /etc/coop/agent.conf to ~/.fboss2/agent.conf
  static ConfigSession& getInstance();

  // Get the path to the session config file (~/.fboss2/agent.conf)
  std::string getSessionConfigPath() const;

  // Get the path to the system config file (/etc/coop/agent.conf)
  std::string getSystemConfigPath() const;

  // Get the path to the CLI config directory (/etc/coop/cli)
  std::string getCliConfigDir() const;

  // Result of a commit operation
  struct CommitResult {
    int revision; // The revision number that was committed
    // Maps each service to the action level that was applied during commit.
    // Services not in this map had no action taken.
    std::map<cli::ServiceType, cli::ConfigActionLevel> actions;
  };

  // Atomically commit the session to /etc/coop/cli/agent-rN.conf,
  // update the symlink /etc/coop/agent.conf to point to it.
  // For HITLESS changes, also calls reloadConfig() on the agent.
  // For AGENT_RESTART changes, does NOT call reloadConfig() - user must restart
  // agent. Returns CommitResult with revision number and action level.
  CommitResult commit(const HostInfo& hostInfo);

  // Rollback to a specific revision or to the previous revision
  // Returns the revision that was rolled back to
  int rollback(const HostInfo& hostInfo);
  int rollback(const HostInfo& hostInfo, const std::string& revision);

  // Check if a session exists
  bool sessionExists() const;

  // Get the parsed agent configuration
  cfg::AgentConfig& getAgentConfig();
  const cfg::AgentConfig& getAgentConfig() const;

  // Get the PortMap for port-to-interface lookups
  utils::PortMap& getPortMap();
  const utils::PortMap& getPortMap() const;

  // Save the configuration back to the session file.
  // Also updates the required action level for the specified service
  // (if the new level is higher than the current one).
  // This combines saving the config and updating its associated metadata.
  void saveConfig(cli::ServiceType service, cli::ConfigActionLevel actionLevel);

  // Extract revision number from a filename or path like "agent-r42.conf"
  // Returns -1 if the filename doesn't match the expected pattern
  static int extractRevisionNumber(const std::string& filenameOrPath);

  // Update the required action level for the current session.
  // Tracks the highest action level across all config commands.
  // Higher action levels take precedence (AGENT_COLDBOOT > AGENT_WARMBOOT >
  // HITLESS).
  void updateRequiredAction(
      cli::ServiceType service,
      cli::ConfigActionLevel actionLevel);

  // Get the current required action level for the session
  cli::ConfigActionLevel getRequiredAction(cli::ServiceType service) const;

  // Reset the required action level to HITLESS (called after successful commit)
  void resetRequiredAction(cli::ServiceType service);

  // Get the systemd service name for a service type
  static std::string getServiceName(cli::ServiceType service);

  // Get the list of commands executed in this session
  const std::vector<std::string>& getCommands() const;

 protected:
  // Constructor for testing with custom paths
  ConfigSession(
      const std::string& sessionConfigPath,
      const std::string& systemConfigPath,
      const std::string& cliConfigDir);

  // Set the singleton instance (for testing only)
  static void setInstance(std::unique_ptr<ConfigSession> instance);

  // Read the command line for the current process from /proc/self/cmdline.
  // Returns the command arguments as a space-separated string,
  // e.g., "config interface eth1/1/1 mtu 9000"
  // Throws runtime_error if the command line cannot be read.
  // Virtual to allow tests to override with mock command lines.
  virtual std::string readCommandLineFromProc() const;

 private:
  std::string sessionConfigPath_;
  std::string systemConfigPath_;
  std::string cliConfigDir_;
  std::string username_;

  // Lazy-initialized configuration and port map
  cfg::AgentConfig agentConfig_;
  std::unique_ptr<utils::PortMap> portMap_;
  bool configLoaded_ = false;

  // Track the highest action level required for pending config changes per
  // service. Persisted to disk so it survives across CLI invocations within a
  // session.
  std::map<cli::ServiceType, cli::ConfigActionLevel> requiredActions_;

  // List of commands executed in this session, persisted to disk
  std::vector<std::string> commands_;

  // Path to the metadata file (e.g., ~/.fboss2/metadata)
  std::string getMetadataPath() const;

  // Load/save metadata (action levels and commands) from disk
  void loadMetadata();
  void saveMetadata();

  // Restart a service via systemd and wait for it to be active
  // For AGENT_WARMBOOT, does a simple restart.
  // For AGENT_COLDBOOT, creates cold_boot_once files before restarting.
  void restartService(cli::ServiceType service, cli::ConfigActionLevel level);

  // Reload config for a service without restart (for HITLESS changes).
  // Each service type has its own reload mechanism.
  void reloadServiceConfig(cli::ServiceType service, const HostInfo& hostInfo);

  // Apply actions (restart or reload) to all services based on their action
  // levels. For WARMBOOT/COLDBOOT, restarts the service. For HITLESS, reloads
  // the config.
  void applyServiceActions(
      const std::map<cli::ServiceType, cli::ConfigActionLevel>& actions,
      const HostInfo& hostInfo);

  // Initialize the session (creates session config file if it doesn't exist)
  void initializeSession();
  void copySystemConfigToSession();
  void loadConfig();
};

} // namespace facebook::fboss
