// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/types.h"

namespace facebook::fboss {

class HwSwitchSyncer;
class HwSwitch;
class SwitchState;
class StateDelta;

class MultiHwSwitchSyncer {
 public:
  MultiHwSwitchSyncer(
      HwSwitch* hwSwitch,
      const std::map<SwitchID, cfg::SwitchInfo>& switchInfoMap);

  ~MultiHwSwitchSyncer();

  void start();

  void stop();

  std::shared_ptr<SwitchState> stateChanged(
      const StateDelta& delta,
      bool transaction);

 private:
  std::map<SwitchID, std::unique_ptr<HwSwitchSyncer>> hwSwitchSyncers_;
  std::atomic<bool> stopped_{true};
};

} // namespace facebook::fboss
