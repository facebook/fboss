/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <memory>
#include <string>
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss::utils {
class PortMap;
}

namespace facebook::fboss {

class ConfigSession {
 public:
  ConfigSession();
  ~ConfigSession() = default;

  // Get or create the current config session
  // If no session exists, copies /etc/coop/agent.conf to ~/.fboss2/agent.conf
  static ConfigSession& getInstance();

  // Get the path to the session config file (~/.fboss2/agent.conf)
  std::string getSessionConfigPath() const;

  // Get the path to the system config file (/etc/coop/agent.conf)
  std::string getSystemConfigPath() const;

  // Atomically commit the session to /etc/coop/cli/agent-rN.conf,
  // update the symlink /etc/coop/agent.conf to point to it, and reload config.
  // Returns the revision number that was committed if the commit was
  // successful.
  int commit(const HostInfo& hostInfo);

  // Check if a session exists
  bool sessionExists() const;

  // Get the parsed agent configuration
  cfg::AgentConfig& getAgentConfig();
  const cfg::AgentConfig& getAgentConfig() const;

  // Get the PortMap for port-to-interface lookups
  utils::PortMap& getPortMap();
  const utils::PortMap& getPortMap() const;

  // Save the configuration back to the session file
  void saveConfig();

  // Extract revision number from a filename or path like "agent-r42.conf"
  // Returns -1 if the filename doesn't match the expected pattern
  static int extractRevisionNumber(const std::string& filenameOrPath);

 protected:
  // Constructor for testing with custom paths
  ConfigSession(
      const std::string& sessionConfigPath,
      const std::string& systemConfigPath,
      const std::string& cliConfigDir);

 private:
  std::string sessionConfigPath_;
  std::string systemConfigPath_;
  std::string cliConfigDir_;
  std::string username_;

  // Lazy-initialized configuration and port map
  cfg::AgentConfig agentConfig_;
  std::unique_ptr<utils::PortMap> portMap_;
  bool configLoaded_ = false;

  // Initialize the session (creates session config file if it doesn't exist)
  void initializeSession();
  void copySystemConfigToSession();
  void loadConfig();
};

} // namespace facebook::fboss
