// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/agent/gen-cpp2/switch_state_types.h"

DECLARE_bool(can_warm_boot);
DECLARE_string(thrift_switch_state_file);

namespace facebook::fboss {

class SwitchState;
class RoutingInformationBase;

class SwSwitchWarmBootHelper {
 public:
  explicit SwSwitchWarmBootHelper(const std::string& warmBootDir);
  bool canWarmBoot() const;
  void storeWarmBootState(const state::WarmbootState& switchStateThrift);
  state::WarmbootState getWarmBootState() const;
  std::string warmBootThriftSwitchStateFile() const;
  const std::string& warmBootDir() const {
    return warmBootDir_;
  }

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

  const std::string warmBootDir_;
  bool canWarmBoot_{false};
};

} // namespace facebook::fboss
