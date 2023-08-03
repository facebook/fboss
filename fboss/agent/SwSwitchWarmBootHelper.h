// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/agent/gen-cpp2/switch_state_types.h"

DECLARE_bool(can_warm_boot);
DECLARE_string(thrift_switch_state_file);

namespace facebook::fboss {

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

 private:
  bool checkAndClearWarmBootFlags();
  std::string forceColdBootOnceFlag() const;

  const std::string warmBootDir_;
  bool canWarmBoot_{false};
};

} // namespace facebook::fboss
