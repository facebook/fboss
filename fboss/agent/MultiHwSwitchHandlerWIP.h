// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/types.h"

#include <folly/futures/Future.h>
#include <memory>

namespace facebook::fboss {

class HwSwitchHandlerWIP;
class SwitchState;
class StateDelta;
struct HwSwitchStateUpdate;

using HwSwitchHandlerInitFn = std::function<std::unique_ptr<HwSwitchHandlerWIP>(
    const SwitchID& switchId,
    const cfg::SwitchInfo& info)>;

class MultiHwSwitchHandlerWIP {
 public:
  MultiHwSwitchHandlerWIP(
      const std::map<int64_t, cfg::SwitchInfo>& switchInfoMap,
      HwSwitchHandlerInitFn hwSwitchHandlerInitFn);

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
