// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"

extern "C" {
#include <bcm/l2.h>
}

namespace facebook::fboss {

class BcmSwitch;
class SwitchState;

class BcmSwitchSettings {
 public:
  explicit BcmSwitchSettings(BcmSwitch* hw) : hw_(hw) {}
  ~BcmSwitchSettings() {}

  std::optional<cfg::L2LearningMode> getL2LearningMode() const {
    return l2LearningMode_;
  }

  void setL2LearningMode(cfg::L2LearningMode l2LearningMode);

  void enableL2LearningCallback();
  void disableL2LearningCallback();

  void setQcmEnable(
      bool qcmEnable,
      const std::shared_ptr<SwitchState>& swState);

 private:
  void enableL2LearningHardware();
  void enableL2LearningSoftware();

  void enablePendingEntriesOnUnknownSrcL2();
  void disablePendingEntriesOnUnknownSrcL2();

  BcmSwitch* hw_;

  std::optional<cfg::L2LearningMode> l2LearningMode_{std::nullopt};
  bool l2AddrCallBackRegisterd_{false};
  std::optional<bool> qcmEnable_{std::nullopt};
};

} // namespace facebook::fboss
