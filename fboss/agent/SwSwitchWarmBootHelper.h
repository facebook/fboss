// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/agent/gen-cpp2/switch_state_types.h"

DECLARE_bool(can_warm_boot);
DECLARE_string(thrift_switch_state_file);

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

  static std::pair<
      std::shared_ptr<SwitchState>,
      std::unique_ptr<RoutingInformationBase>>
  reconstructStateAndRib(
      std::optional<state::WarmbootState> wbState,
      bool hasL3);

 private:
  bool checkAndClearWarmBootFlags();
  std::string forceColdBootOnceFlag() const;
  std::string forceColdBootOnceFlagLegacy() const;
  std::string warmBootFlag() const;
  std::string warmBootFlagLegacy() const;
  void setCanWarmBoot();

  const AgentDirectoryUtil* directoryUtil_;
  const std::string warmBootDir_;
  bool canWarmBoot_{false};
  HwAsicTable* asicTable_;
};

} // namespace facebook::fboss
