// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>

namespace facebook::fboss {

class HwSwitch;
class StateDelta;

struct HwSwitchStateUpdate {
  HwSwitchStateUpdate(const StateDelta& delta, bool transaction);
  std::shared_ptr<SwitchState> oldState;
  std::shared_ptr<SwitchState> newState;
  fsdb::OperDelta inDelta;
  bool isTransaction;
};

class HwSwitchSyncer {
 public:
  HwSwitchSyncer(
      HwSwitch* hwSwitch,
      const SwitchID& switchId,
      const cfg::SwitchInfo& info);

  void start();

  ~HwSwitchSyncer();

  folly::Future<std::shared_ptr<SwitchState>> stateChanged(
      HwSwitchStateUpdate update);

 private:
  std::shared_ptr<SwitchState> stateChangedImpl(
      const HwSwitchStateUpdate& update);

  fsdb::OperDelta stateChangedImpl(
      const fsdb::OperDelta& delta,
      bool transaction);

  void run();

  void stop();

  HwSwitch* hwSwitch_;
  SwitchID switchId_;
  cfg::SwitchInfo info_;
  folly::EventBase hwSwitchManagerEvb_;
  std::unique_ptr<std::thread> hwSwitchManagerThread_;
  OperDeltaFilter operDeltaFilter_;
};

} // namespace facebook::fboss
