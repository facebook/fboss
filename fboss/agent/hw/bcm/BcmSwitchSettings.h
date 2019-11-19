// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"

extern "C" {
#include <opennsl/l2.h>
}

namespace facebook {
namespace fboss {
class BcmSwitch;

class BcmSwitchSettings {
 public:
  explicit BcmSwitchSettings(BcmSwitch* hw) : hw_(hw) {}
  ~BcmSwitchSettings() {}

  void setL2LearningMode(cfg::L2LearningMode l2LearningMode);

 private:
  void enableL2LearningHardware();
  void enableL2LearningSoftware();

  void enableL2LearningCallback();
  void disableL2LearningCallback();

  void enablePendingEntriesOnUnknownSrcL2();
  void disablePendingEntriesOnUnknownSrcL2();

  BcmSwitch* hw_;

  std::optional<cfg::L2LearningMode> l2LearningMode_{std::nullopt};
  bool l2AddrCallBackRegisterd_{false};
};

} // namespace fboss
} // namespace facebook
