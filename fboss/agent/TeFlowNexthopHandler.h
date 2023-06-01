// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/StateObserver.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {
class SwSwitch;

class TeFlowNexthopHandler : public StateObserver {
 public:
  explicit TeFlowNexthopHandler(SwSwitch* sw);
  ~TeFlowNexthopHandler() override;

  void stateUpdated(const StateDelta& delta) override;

 private:
  std::shared_ptr<SwitchState> handleUpdate(
      const std::shared_ptr<SwitchState>& state);
  std::shared_ptr<SwitchState> updateTeFlowEntries(
      const std::shared_ptr<SwitchState>& newState);
  bool updateTeFlowEntry(
      const std::shared_ptr<TeFlowEntry>& origTeFlowEntry,
      std::shared_ptr<SwitchState>& newState);
  bool hasTeFlowChanges(const StateDelta& delta);

  SwSwitch* sw_;
};

} // namespace facebook::fboss
