/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AgentDirectoryUtil.h"

#include <folly/logging/xlog.h>
#include <string>

DEFINE_string(
    crash_switch_state_file,
    "crash_switch_state",
    "File for dumping SwitchState state on crash");

DEFINE_string(
    crash_thrift_switch_state_file,
    "crash_thrift_switch_state",
    "File for dumping SwitchState thrift on crash");

DEFINE_string(
    crash_hw_state_file,
    "crash_hw_state",
    "File for dumping HW state on crash");

DEFINE_string(
    hw_config_file,
    "hw_config",
    "File for dumping HW config on startup");

DEFINE_string(
    volatile_state_dir,
    "/dev/shm/fboss",
    "Directory for storing volatile state");
DEFINE_string(
    persistent_state_dir,
    "/var/facebook/fboss",
    "Directory for storing persistent state");

DEFINE_string(
    volatile_state_dir_phy,
    "/dev/shm/fboss/qsfp_service/phy",
    "Directory for storing phy volatile state");
DEFINE_string(
    persistent_state_dir_phy,
    "/var/facebook/fboss/qsfp_service/phy",
    "Directory for storing phy persistent state");

using namespace std;

namespace facebook::fboss {

AgentDirectoryUtil::AgentDirectoryUtil(
    std::string volatileStateDir,
    std::string persistentStateDir)
    : volatileStateDir_(std::move(volatileStateDir)),
      persistentStateDir_(std::move(persistentStateDir)) {}

string AgentDirectoryUtil::getVolatileStateDir() const {
  return volatileStateDir_;
}

string AgentDirectoryUtil::getPersistentStateDir() const {
  return persistentStateDir_;
}

std::string AgentDirectoryUtil::getCrashHwStateFile() const {
  return getCrashInfoDir() + "/" + FLAGS_crash_hw_state_file;
}

std::string AgentDirectoryUtil::getCrashSwitchStateFile() const {
  return getCrashInfoDir() + "/" + FLAGS_crash_switch_state_file;
}

std::string AgentDirectoryUtil::getCrashThriftSwitchStateFile() const {
  return getCrashInfoDir() + "/" + FLAGS_crash_thrift_switch_state_file;
}

} // namespace facebook::fboss
