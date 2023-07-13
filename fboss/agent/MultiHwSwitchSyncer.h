// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/types.h"

#include <folly/futures/Future.h>

namespace facebook::fboss {

class HwSwitchSyncer;
struct HwSwitchHandler;
class SwitchState;
class StateDelta;
struct HwSwitchStateUpdate;

class MultiHwSwitchSyncer {
 public:
  MultiHwSwitchSyncer(
      HwSwitchHandler* hwSwitchHandler,
      const std::map<SwitchID, cfg::SwitchInfo>& switchInfoMap);

  ~MultiHwSwitchSyncer();

  void start();

  void stop();

  std::shared_ptr<SwitchState> stateChanged(
      const StateDelta& delta,
      bool transaction);

 private:
  folly::Future<std::shared_ptr<SwitchState>> stateChanged(
      SwitchID switchId,
      const HwSwitchStateUpdate& update);

  std::shared_ptr<SwitchState> getStateUpdateResult(
      SwitchID switchId,
      folly::Future<std::shared_ptr<SwitchState>>&& future);

  std::map<SwitchID, std::unique_ptr<HwSwitchSyncer>> hwSwitchSyncers_;
  std::atomic<bool> stopped_{true};
};

} // namespace facebook::fboss
