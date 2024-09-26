/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <gflags/gflags.h>
#include <memory>
#include <string>

DECLARE_string(crash_switch_state_file);
DECLARE_string(crash_thrift_switch_state_file);
DECLARE_string(crash_hw_state_file);
DECLARE_string(hw_config_file);
DECLARE_string(volatile_state_dir);
DECLARE_string(persistent_state_dir);
DECLARE_string(volatile_state_dir_phy);
DECLARE_string(persistent_state_dir_phy);

namespace facebook::fboss {
class AgentDirectoryUtil {
 public:
  AgentDirectoryUtil();

  AgentDirectoryUtil(
      const std::string& volatileStateDir,
      const std::string& persistentStateDir);

  AgentDirectoryUtil(
      const std::string& volatileStateDir,
      const std::string& persistentStateDir,
      const std::string& packageDirectory,
      const std::string& systemdDirectory,
      const std::string& configDirectory,
      const std::string& drainConfigDirectory);

  /*
   * Get the path to a directory where persistent state can be stored.
   *
   * Files written to this directory should be preserved across system reboots.
   */
  std::string getPersistentStateDir() const;

  /*
   * Get the path to a directory where volatile state can be stored.
   *
   * Files written to this directory should be preserved across controller
   * restarts, but must be removed across system restarts.
   *
   * For instance, these files could be stored in a ramdisk.  Alternatively,
   * these could be stored in persistent storage with an init script that
   * empties the directory on reboot.
   */
  std::string getVolatileStateDir() const;

  /*
   * Get the directory where we will dump info when there is a crash.
   *
   * The directory is in persistent storage.
   */
  std::string getCrashInfoDir() const {
    return getPersistentStateDir() + "/crash";
  }

  /*
   * Directory where we store info about state updates that led to a crash
   */
  std::string getCrashBadStateUpdateDir() const {
    return getCrashInfoDir() + "/bad_update";
  }

  std::string getCrashBadStateUpdateOldStateFile() const {
    return getCrashBadStateUpdateDir() + "/old_state";
  }

  std::string getCrashBadStateUpdateNewStateFile() const {
    return getCrashBadStateUpdateDir() + "/new_state";
  }

  /*
   * Get location we dump the running config of the switch
   */
  std::string getRunningConfigDumpFile() const {
    return getPersistentStateDir() + "/running-agent.conf";
  }

  /*
   * Get the directory where warm boot state is stored.
   */
  std::string getWarmBootDir() const {
    return getVolatileStateDir() + "/warm_boot";
  }

  /*
   * Get filename for where we dump hw state on crash
   */
  std::string getCrashHwStateFile() const;

  /*
   * Get filename for where we dump switch state on crash
   */
  std::string getCrashSwitchStateFile() const;

  /*
   * Get filename for where we dump thrift switch state on crash
   */
  std::string getCrashThriftSwitchStateFile() const;

  std::string getColdBootOnceFile() const;

  std::string getSwColdBootOnceFile() const;

  std::string getHwColdBootOnceFile(int switchIndex) const;

  std::string getSwColdBootOnceFile(const std::string& shmDir) const;

  std::string getHwColdBootOnceFile(const std::string& shmDir, int switchIndex)
      const;

  std::string getColdBootOnceFile(const std::string& shmDir) const;

  std::string getRoutingProtocolColdBootDrainTimeFile() const;

  std::string getRoutingProtocolColdBootDrainTimeFile(int switchIndex) const;

  std::string getSwSwitchCanWarmBootFile() const;

  std::string getHwSwitchCanWarmBootFile(int switchIndex) const;

  std::string getPackageDirectory() const;

  std::string getMultiSwitchScriptsDirectory() const;

  std::string getSystemdDirectory() const;

  std::string getConfigDirectory() const;

  std::string getDrainConfigDirectory() const;

  std::string getAgentLiveConfig() const;

  std::string getAgentDrainConfig() const;

  std::string getSwAgentServicePath() const;

  std::string getSwAgentServiceSymLink() const;

  std::string getHwAgentServiceTemplatePath() const;

  std::string getHwAgentServiceInstance(int switchIndex) const;

  std::string getHwAgentServiceTemplateSymLink() const;

  std::string getHwAgentServiceInstanceSymLink(int switchIndex) const;

  std::string getUndrainedFlag() const;

  std::string getStartupConfig() const;

  std::string getMultiSwitchPreStartScript() const;

  std::string getPreStartShellScript() const;

  // used in wrapper testing to make agent sleep for 5 seconds
  std::string sleepSwSwitchOnSigTermFile() const;

  std::string sleepHwSwitchOnSigTermFile(int switchIndex) const;

  std::string getMaxPostSignalWaitTimeFile() const;

  std::string getWrapperRefactorFlag() const;

  std::string exitTimeFile(const std::string& processName) const;

  std::string restartDurationFile(const std::string& processName) const;

  std::string pidFile(const std::string& name) const;

  std::string exitSwSwitchForColdBootFile() const;

  std::string exitHwSwitchForColdBootFile(int switchIndex) const;

  std::string agentEnsembleConfigDir() const;

  std::string getTestHwAgentConfigFile(int switchIndex) const;

 private:
  const std::string volatileStateDir_;
  const std::string persistentStateDir_;
  const std::string packageDirectory_;
  const std::string systemdDirectory_;
  const std::string configDirectory_;
  const std::string drainConfigDirectory_;
};

} // namespace facebook::fboss
