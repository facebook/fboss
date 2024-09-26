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

namespace {
static auto constexpr kPkgDir =
    "/etc/packages/neteng-fboss-wedge_agent/current";
static auto constexpr kSystemdDir = "/etc/systemd/system";
static auto constexpr kCoopAgentDir = "/etc/coop/agent";
static auto constexpr kCoopAgentDrainDir = "/etc/coop/agent_drain";
static auto constexpr kSwAgentService = "fboss_sw_agent.service";
static auto constexpr kHwAgentService = "fboss_hw_agent@.service";
} // namespace

namespace facebook::fboss {

AgentDirectoryUtil::AgentDirectoryUtil(
    const std::string& volatileStateDir,
    const std::string& persistentStateDir)
    : AgentDirectoryUtil(
          volatileStateDir,
          persistentStateDir,
          kPkgDir, // packageDirectory_
          kSystemdDir, // systemdDirectory_
          kCoopAgentDir, // configDirectory_
          kCoopAgentDrainDir // drainConfigDirectory_
      ) {}

AgentDirectoryUtil::AgentDirectoryUtil()
    : AgentDirectoryUtil(FLAGS_volatile_state_dir, FLAGS_persistent_state_dir) {
}

AgentDirectoryUtil::AgentDirectoryUtil(
    const std::string& volatileStateDir,
    const std::string& persistentStateDir,
    const std::string& packageDirectory,
    const std::string& systemdDirectory,
    const std::string& configDirectory,
    const std::string& drainConfigDirectory)
    : volatileStateDir_(volatileStateDir),
      persistentStateDir_(persistentStateDir),
      packageDirectory_(packageDirectory),
      systemdDirectory_(systemdDirectory),
      configDirectory_(configDirectory),
      drainConfigDirectory_(drainConfigDirectory) {}

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

std::string AgentDirectoryUtil::getSwColdBootOnceFile() const {
  return getSwColdBootOnceFile(getWarmBootDir());
}

std::string AgentDirectoryUtil::getHwColdBootOnceFile(int switchIndex) const {
  return getHwColdBootOnceFile(getWarmBootDir(), switchIndex);
}

std::string AgentDirectoryUtil::getSwColdBootOnceFile(
    const std::string& shmDir) const {
  return shmDir + "/" + "sw_cold_boot_once";
}

std::string AgentDirectoryUtil::getHwColdBootOnceFile(
    const std::string& shmDir,
    int switchIndex) const {
  return shmDir + "/" + "hw_cold_boot_once_" +
      folly::to<std::string>(switchIndex);
}

std::string AgentDirectoryUtil::getColdBootOnceFile() const {
  return getColdBootOnceFile(getWarmBootDir());
}

std::string AgentDirectoryUtil::getColdBootOnceFile(
    const std::string& shmDir) const {
  return shmDir + "/" + "cold_boot_once_0";
}

std::string AgentDirectoryUtil::getRoutingProtocolColdBootDrainTimeFile()
    const {
  return getVolatileStateDir() + "/routing_protocol_cold_boot_drain_time";
}

std::string AgentDirectoryUtil::getRoutingProtocolColdBootDrainTimeFile(
    int switchIndex) const {
  return getVolatileStateDir() + "/routing_protocol_cold_boot_drain_time_" +
      folly::to<std::string>(switchIndex);
}

std::string AgentDirectoryUtil::getSwSwitchCanWarmBootFile() const {
  return getWarmBootDir() + "/can_warm_boot";
}

std::string AgentDirectoryUtil::getPackageDirectory() const {
  return packageDirectory_;
}

std::string AgentDirectoryUtil::getMultiSwitchScriptsDirectory() const {
  return packageDirectory_ + "/multi_switch_agent_scripts";
}

std::string AgentDirectoryUtil::getSystemdDirectory() const {
  return systemdDirectory_;
}

std::string AgentDirectoryUtil::getConfigDirectory() const {
  return configDirectory_;
}

std::string AgentDirectoryUtil::getDrainConfigDirectory() const {
  return drainConfigDirectory_;
}

std::string AgentDirectoryUtil::getAgentLiveConfig() const {
  return getConfigDirectory() + "/current";
}

std::string AgentDirectoryUtil::getAgentDrainConfig() const {
  return getDrainConfigDirectory() + "/current";
}

std::string AgentDirectoryUtil::getSwAgentServicePath() const {
  return getMultiSwitchScriptsDirectory() + "/" + kSwAgentService;
}

std::string AgentDirectoryUtil::getSwAgentServiceSymLink() const {
  return getSystemdDirectory() + "/" + kSwAgentService;
}

std::string AgentDirectoryUtil::getHwAgentServiceTemplatePath() const {
  return getMultiSwitchScriptsDirectory() + "/" + kHwAgentService;
}

std::string AgentDirectoryUtil::getHwAgentServiceInstance(
    int switchIndex) const {
  return "fboss_hw_agent@" + folly::to<std::string>(switchIndex) + ".service";
}

std::string AgentDirectoryUtil::getHwAgentServiceTemplateSymLink() const {
  return getSystemdDirectory() + "/" + kHwAgentService;
}

std::string AgentDirectoryUtil::getHwAgentServiceInstanceSymLink(
    int switchIndex) const {
  return getSystemdDirectory() + "/" + getHwAgentServiceInstance(switchIndex);
}

std::string AgentDirectoryUtil::getUndrainedFlag() const {
  return getVolatileStateDir() + "/UNDRAINED";
}

std::string AgentDirectoryUtil::getStartupConfig() const {
  return getVolatileStateDir() + "/agent_startup_config";
}

std::string AgentDirectoryUtil::getMultiSwitchPreStartScript() const {
  return getMultiSwitchScriptsDirectory() + "/pre_multi_switch_agent_start.par";
}

std::string AgentDirectoryUtil::getPreStartShellScript() const {
  return getVolatileStateDir() + "/pre_start.sh";
}

std::string AgentDirectoryUtil::sleepSwSwitchOnSigTermFile() const {
  return getVolatileStateDir() + "/sw_sleep_on_sigterm";
}

std::string AgentDirectoryUtil::sleepHwSwitchOnSigTermFile(
    int switchIndex) const {
  return getVolatileStateDir() + "/hw_sleep_on_sigterm" +
      folly::to<std::string>(switchIndex);
}

std::string AgentDirectoryUtil::getMaxPostSignalWaitTimeFile() const {
  return getVolatileStateDir() + "/max_post_signal_wait_time_wedge_agent";
}

std::string AgentDirectoryUtil::getWrapperRefactorFlag() const {
  return "/etc/fboss/features/cpp_wedge_agent_wrapper/current/on";
}

std::string AgentDirectoryUtil::exitTimeFile(
    const std::string& processName) const {
  return "/dev/shm/" + processName + "_last_exit_time";
}

std::string AgentDirectoryUtil::restartDurationFile(
    const std::string& processName) const {
  return "/dev/shm/" + processName + "_last_restart_duration";
}

std::string AgentDirectoryUtil::pidFile(const std::string& name) const {
  return getVolatileStateDir() + "/." + name + ".pid";
}

std::string AgentDirectoryUtil::exitSwSwitchForColdBootFile() const {
  return folly::to<std::string>(
      getVolatileStateDir(), "/exit_sw_for_cold_boot");
}

std::string AgentDirectoryUtil::exitHwSwitchForColdBootFile(
    int switchIndex) const {
  return folly::to<std::string>(
      getVolatileStateDir(), "/exit_hw_for_cold_boot_", switchIndex);
}

std::string AgentDirectoryUtil::getHwSwitchCanWarmBootFile(
    int switchIndex) const {
  return folly::to<std::string>(
      getWarmBootDir(), "/can_warm_boot_", switchIndex);
}

std::string AgentDirectoryUtil::agentEnsembleConfigDir() const {
  return folly::to<std::string>(getPersistentStateDir(), "/agent_ensemble/");
}

std::string AgentDirectoryUtil::getTestHwAgentConfigFile(
    int switchIndex) const {
  return folly::to<std::string>(
      agentEnsembleConfigDir(), "hw_agent_test_", switchIndex, ".conf");
}
} // namespace facebook::fboss
