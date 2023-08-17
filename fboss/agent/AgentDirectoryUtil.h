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
  AgentDirectoryUtil(
      std::string volatileStateDir,
      std::string persistentStateDir);
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

 private:
  std::string volatileStateDir_;
  std::string persistentStateDir_;
};

} // namespace facebook::fboss
