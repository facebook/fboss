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
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/Git.h"
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
  ~ConfigSession() = default;

  // Get or create the current config session
  // If no session exists, copies /etc/coop/agent.conf to ~/.fboss2/agent.conf
  static ConfigSession& getInstance();

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
    cli::ConfigActionLevel actionLevel; // The action level that was required
    // Note: configReloaded can be inferred from actionLevel:
    // - HITLESS: config was reloaded via reloadConfig()
    // - AGENT_RESTART: agent was restarted via systemd
  };

  // Atomically commit the session to /etc/coop/cli/agent.conf and create a git
  // commit. For HITLESS changes, also calls reloadConfig() on the agent.
  // For AGENT_RESTART changes, restarts the agent via systemd.
  // Returns CommitResult with git commit SHA and action level.
  CommitResult commit(const HostInfo& hostInfo);

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
  // If actionLevel is provided, also updates the required action level
  // for the specified agent (if the new level is higher than the current one).
  // This combines saving the config and updating its associated metadata.
  void saveConfig(
      std::optional<cli::ConfigActionLevel> actionLevel = std::nullopt,
      cli::AgentType agent = cli::AgentType::WEDGE_AGENT);

  // Get the Git instance for this config session
  // Used to access the Git repository for history, rollback, etc.
  Git& getGit();
  const Git& getGit() const;

  // Update the required action level for the current session.
  // Tracks the highest action level across all config commands.
  // Higher action levels take precedence (AGENT_RESTART > HITLESS).
  // The agent parameter specifies which agent this action level applies to.
  // Currently only WEDGE_AGENT is supported; future agents will be added.
  void updateRequiredAction(
      cli::ConfigActionLevel actionLevel,
      cli::AgentType agent = cli::AgentType::WEDGE_AGENT);

  // Get the current required action level for the session
  // The agent parameter specifies which agent to get the action level for.
  cli::ConfigActionLevel getRequiredAction(
      cli::AgentType agent = cli::AgentType::WEDGE_AGENT) const;

  // Reset the required action level to HITLESS (called after successful commit)
  // The agent parameter specifies which agent to reset the action level for.
  void resetRequiredAction(cli::AgentType agent = cli::AgentType::WEDGE_AGENT);

  // Get the list of commands executed in this session
  const std::vector<std::string>& getCommands() const;

 protected:
  // Constructor for testing with custom paths
  ConfigSession(std::string sessionConfigDir, std::string systemConfigDir);

  // Set the singleton instance (for testing only)
  static void setInstance(std::unique_ptr<ConfigSession> instance);

  // Add a command to the history (for testing only)
  // This allows tests to simulate command tracking without /proc/self/cmdline
  void addCommand(const std::string& command);

 private:
  std::string sessionConfigDir_; // Typically ~/.fboss2
  std::string systemConfigDir_; // Typically /etc/coop
  std::string username_;

  // Git instance for version control operations
  std::unique_ptr<Git> git_;

  // Lazy-initialized configuration and port map
  cfg::AgentConfig agentConfig_;
  std::unique_ptr<utils::PortMap> portMap_;
  bool configLoaded_ = false;

  // Track the highest action level required for pending config changes per
  // agent. Persisted to disk so it survives across CLI invocations within a
  // session.
  std::map<cli::AgentType, cli::ConfigActionLevel> requiredActions_;

  // List of commands executed in this session, persisted to disk
  std::vector<std::string> commands_;

  // Path to the system metadata file (in the Git repo)
  std::string getSystemMetadataPath() const;

  // Path to the session metadata file (in the user's home directory)
  std::string getMetadataPath() const;

  // Load/save metadata (action levels and commands) from disk
  void loadMetadata();
  void saveMetadata();

  // Restart an agent via systemd and wait for it to be active
  void restartAgent(cli::AgentType agent);

  // Get the systemd service name for an agent
  static std::string getServiceName(cli::AgentType agent);

  // Initialize the session (creates session config file if it doesn't exist)
  void initializeSession();
  void copySystemConfigToSession();
  void loadConfig();

  // Initialize the Git repository if needed
  void initializeGit();
};

} // namespace facebook::fboss
