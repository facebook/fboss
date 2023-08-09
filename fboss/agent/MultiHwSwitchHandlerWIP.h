// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/types.h"

#include <folly/futures/Future.h>

namespace facebook::fboss {

class HwSwitchHandlerWIP;
struct HwSwitchHandlerDeprecated;
class SwitchState;
class StateDelta;
struct HwSwitchStateUpdate;

class MultiHwSwitchHandlerWIP {
 public:
  MultiHwSwitchHandlerWIP(
      HwSwitchHandlerDeprecated* hwSwitchHandler,
      const std::map<SwitchID, cfg::SwitchInfo>& switchInfoMap);

  ~MultiHwSwitchHandlerWIP();

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

  std::map<SwitchID, std::unique_ptr<HwSwitchHandlerWIP>> hwSwitchSyncers_;
  std::atomic<bool> stopped_{true};
};

} // namespace facebook::fboss
