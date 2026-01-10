// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/agent/gen-cpp2/switch_state_types.h"

namespace facebook::fboss {

class SwitchState;
class RoutingInformationBase;
class AgentDirectoryUtil;
class HwAsicTable;
class HwSwitchThriftClientTable;

class SwSwitchWarmBootHelper {
 public:
  explicit SwSwitchWarmBootHelper(
      const AgentDirectoryUtil* directoryUtil,
      HwAsicTable* table);
  bool canWarmBoot(
      bool isRunModeMultiSwitch,
      HwSwitchThriftClientTable* hwSwitchThriftClientTable);
  void storeWarmBootState(const state::WarmbootState& switchStateThrift);
  state::WarmbootState getWarmBootState() const;
  std::string warmBootThriftSwitchStateFile() const;
  const std::string& warmBootDir() const {
    return warmBootDir_;
  }
  void logBoot(
      const std::string& bootType,
      const std::string& sdkVersion,
      const std::string& agentVersion);

 private:
  void setCanWarmBoot();
  bool canWarmBootFromFile() const;
  bool canWarmBootFromThrift(
      bool isRunModeMultiSwitch,
      HwSwitchThriftClientTable* hwSwitchThriftClientTable);

  const AgentDirectoryUtil* directoryUtil_;
  const HwAsicTable* asicTable_;
  const std::string warmBootDir_;
  bool forceColdBootFlag_{false};
  bool canWarmBootFlag_{false};
  bool asicCanWarmBoot_{false};
  std::optional<state::WarmbootState> recoveredStateFromHW_;
};

} // namespace facebook::fboss
