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
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/Git.h"
#include "fboss/cli/fboss2/session/SystemdInterface.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/PortMap.h"

namespace facebook::fboss {

/**
 * ConfigSession manages configuration editing sessions for the fboss2 CLI.
 *
 * OVERVIEW:
 * ConfigSession provides a session-based workflow for editing FBOSS agent
 * configuration. It maintains a session file that can be edited and then
 * atomically committed to the system configuration with Git version control.
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
 *      a. Atomically writes the session config to /etc/coop/cli/agent.conf
 *      b. Ensure /etc/coop/agent.conf is a symlink to /etc/coop/cli/agent.conf
 *      c. Creates a Git commit with the updated agent.conf and metadata
 *      d. Calls reloadConfig() on wedge_agent (or restarts it for
 *         AGENT_RESTART changes)
 *   3. The session file is cleared (ready for next edit session)
 *
 * ROLLBACK FLOW:
 * To revert to a previous configuration:
 *   1. User runs: fboss2-dev config rollback [<revision>]
 *   2. ConfigSession::rollback() is called, which:
 *      a. Reads the target revision's agent.conf from Git history
 *      b. Atomically writes it to /etc/coop/cli/agent.conf
 *      c. Creates a new Git commit indicating the rollback
 *      d. Calls wedge_agent to reload the configuration (or restarts
 *         it if necessary)
 *
 * CONFIGURATION FILES:
 * - Session file: ~/.fboss2/agent.conf (per-user, temporary edits)
 * - System config: /etc/coop/agent.conf (symlink to real config, Git-versioned)
 * - CLI config: /etc/coop/cli/agent.conf (actual config file, Git-versioned)
 * - Metadata: /etc/coop/cli/cli_metadata.json (commit metadata, Git-versioned)
 *
 * VERSION CONTROL:
 * The /etc/coop directory is a local Git repository. Each commit() creates
 * a Git commit with the updated config. History is retrieved via git log,
 * and rollback reads from Git history rather than using git revert.
 *
 * THREAD SAFETY:
 * ConfigSession is NOT thread-safe. It is designed for single-threaded CLI
 * command execution. The code is safe in face of concurrent usage from
 * multiple processes.
 */
class ConfigSession {
 public:
  ConfigSession();
  virtual ~ConfigSession() = default;

  // Get or create the current config session
  // If no session exists, copies /etc/coop/agent.conf to ~/.fboss2/agent.conf
  static ConfigSession& getInstance();

  // Static path getters - can be called without creating a session instance.
  // These are useful for checking if session files exist without triggering
  // session initialization.
  static std::string getSessionDir();
  static std::string getSessionConfigPathStatic();
  static std::string getSessionMetadataPathStatic();

  // Get the path to the session config file (~/.fboss2/agent.conf)
  std::string getSessionConfigPath() const;

  // Get the path to the system config file (/etc/coop/agent.conf symlink)
  std::string getSystemConfigPath() const;

  // Get the path to the CLI config directory (/etc/coop/cli)
  std::string getCliConfigDir() const;

  // Get the path to the actual CLI config file (/etc/coop/cli/agent.conf)
  std::string getCliConfigPath() const;

  // Result of a commit operation
  struct CommitResult {
    std::string commitSha; // The git commit SHA of the committed config
    // Maps each service to the action level that was applied during commit.
    // Services not in this map had no action taken.
    std::map<cli::ServiceType, cli::ConfigActionLevel> actions;
    // Maps each service to the list of actual systemd service names that were
    // restarted/reloaded (e.g., "fboss_sw_agent", "fboss_hw_agent@0", etc.)
    std::map<cli::ServiceType, std::vector<std::string>> serviceNames;
  };

  // Atomically commit the session to /etc/coop/cli/agent.conf and create a git
  // commit. For HITLESS changes, also calls reloadConfig() on the agent.
  // For AGENT_RESTART changes, restarts the agent via systemd.
  // Returns CommitResult with git commit SHA and action level.
  CommitResult commit(const HostInfo& hostInfo);

  // Rebase the session onto the current HEAD.
  // This is needed when someone else has committed changes while this session
  // was in progress. It computes the diff between the base config and the
  // session config, then applies that diff on top of the current HEAD.
  // Throws std::runtime_error if there are conflicts that cannot be resolved.
  void rebase();

  // Rollback to a specific revision (git commit SHA) or to the previous
  // revision Returns the git commit SHA of the new commit created for the
  // rollback
  std::string rollback(const HostInfo& hostInfo);
  std::string rollback(const HostInfo& hostInfo, const std::string& commitSha);

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
  // Save the configuration for AGENT service with HITLESS action level.
  void saveConfig();

  // Get the Git instance for this config session
  // Used to access the Git repository for history, rollback, etc.
  Git& getGit();
  const Git& getGit() const;

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
  ConfigSession(std::string sessionConfigDir, std::string systemConfigDir);

  // Constructor for testing with custom paths and mock systemd
  ConfigSession(
      std::string sessionConfigDir,
      std::string systemConfigDir,
      std::unique_ptr<SystemdInterface> systemd);

  // Set the singleton instance (for testing only)
  static void setInstance(std::unique_ptr<ConfigSession> instance);

  // Read the command line for the current process from /proc/self/cmdline.
  // Returns the command arguments as a space-separated string,
  // e.g., "config interface eth1/1/1 mtu 9000"
  // Throws runtime_error if the command line cannot be read.
  // Virtual to allow tests to override with mock command lines.
  virtual std::string readCommandLineFromProc() const;

  // Detect if running in split mode by checking if fboss_sw_agent is enabled.
  // Returns true if fboss_sw_agent is enabled (split mode), false otherwise
  // (monolithic mode).
  // Virtual to allow tests to override with mock implementations.
  virtual bool isSplitMode() const;

  // Restart a service via systemd and wait for it to be active
  // For AGENT_WARMBOOT, does a simple restart.
  // For AGENT_COLDBOOT, creates cold_boot_once files before restarting.
  // Returns the list of actual systemd service names that were restarted.
  std::vector<std::string> restartService(
      cli::ServiceType service,
      cli::ConfigActionLevel level);

 private:
  std::string sessionConfigDir_; // Typically ~/.fboss2
  std::string systemConfigDir_; // Typically /etc/coop
  std::string username_;

  // Git instance for version control operations
  std::unique_ptr<Git> git_;

  // Systemd interface for service management operations
  std::unique_ptr<SystemdInterface> systemd_;

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

  // Git commit SHA that this session is based on (captured when session is
  // created). Used to detect if someone else committed changes while this
  // session was in progress.
  std::string base_;

  // Path to the system metadata file (in the Git repo)
  std::string getSystemMetadataPath() const;

  // Path to the session metadata file (in the user's home directory)
  std::string getMetadataPath() const;

  // Load/save metadata (action levels and commands) from disk
  void loadMetadata();
  void saveMetadata();

  // Reload config for a service without restart (for HITLESS changes).
  // Each service type has its own reload mechanism.
  // Returns the list of actual systemd service names that were reloaded.
  std::vector<std::string> reloadServiceConfig(
      cli::ServiceType service,
      const HostInfo& hostInfo);

  // Apply actions (restart or reload) to all services based on their action
  // levels. For WARMBOOT/COLDBOOT, restarts the service. For HITLESS, reloads
  // the config.
  // Returns a map of service type to list of actual systemd service names.
  std::map<cli::ServiceType, std::vector<std::string>> applyServiceActions(
      const std::map<cli::ServiceType, cli::ConfigActionLevel>& actions,
      const HostInfo& hostInfo);

  // Helper function to get coldboot marker file path for a single service.
  // Uses AgentDirectoryUtil to get the correct file path for the service.
  // @param service Service name (e.g., "fboss_sw_agent", "fboss_hw_agent@0")
  // @return Coldboot marker file path
  // @throws std::runtime_error if service type is unknown
  static std::string getColdbootFileForService(const std::string& service);

  // Helper function to create a coldboot marker file.
  // Creates parent directories if needed and handles permission errors.
  // @param coldbootFile File path to create
  // @throws std::runtime_error if file creation fails
  static void createColdbootMarkerFile(const std::string& coldbootFile);

  // Helper function to perform coldboot for a list of services.
  // Implements the common coldboot sequence:
  // 1. Stop all services
  // 2. Create coldboot marker files
  // 3. Start all services
  // 4. Wait for all services to become active
  // @param services List of service names to coldboot
  // @param systemd SystemdInterface to use for service operations
  void performColdboot(
      const std::vector<std::string>& services,
      SystemdInterface* systemd);

  // Helper function to perform warmboot for a list of services.
  // Implements the common warmboot sequence:
  // 1. Restart all services
  // 2. Wait for all services to become active
  // @param services List of service names to warmboot
  // @param systemd SystemdInterface to use for service operations
  void performWarmboot(
      const std::vector<std::string>& services,
      SystemdInterface* systemd);

  // Helper function to get the list of services to restart based on mode.
  // Detects split vs monolithic mode and returns the appropriate service list.
  // @param service The service type to restart
  // @return List of systemd service names to restart
  std::vector<std::string> getServicesToRestart(cli::ServiceType service);

  // Initialize the session (creates session config file if it doesn't exist)
  void initializeSession();
  void copySystemConfigToSession() const;
  void loadConfig();

  // Initialize the Git repository if needed
  void initializeGit();
};

} // namespace facebook::fboss
