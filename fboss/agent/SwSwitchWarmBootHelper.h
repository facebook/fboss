// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/agent/gen-cpp2/switch_state_types.h"

namespace facebook::fboss {

class SwitchState;
class RoutingInformationBase;
class AgentDirectoryUtil;
class HwAsicTable;

class SwSwitchWarmBootHelper {
 public:
  explicit SwSwitchWarmBootHelper(
      const AgentDirectoryUtil* directoryUtil,
      HwAsicTable* table);
  bool canWarmBoot() const;
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

  const AgentDirectoryUtil* directoryUtil_;
  const std::string warmBootDir_;
  bool canWarmBoot_{false};
  HwAsicTable* asicTable_;
};

} // namespace facebook::fboss
